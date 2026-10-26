# MCU Integration Lab: A Repeatable Cortex-M Workflow

## Scope and hardware boundary

This chapter turns the book's abstractions into a concrete engineering plan for an STM32-class Cortex-M4F development board. It is a **lab procedure and reference architecture**, not a claim that the complete microphone-to-decision system has been wired, flashed, timed, or acoustically validated for this book. The source repository's C++20 reference functions run on the host and can be compiled into MCU firmware after platform integration; they are not themselves a speech-recognition model.

A suitable first development platform is the NUCLEO-F446RE, which offers a Cortex-M4F CPU and ordinary timer, DMA, debug, and communication peripherals. The particular audio microphone, its electrical interface, and the available peripheral pins must be selected from their datasheets and verified against the board schematic before wiring. The Nucleo board by itself does not provide a microphone. An external digital microphone (PDM or I$^2$S, where supported and correctly wired) or a conditioned analog microphone with ADC sampling is needed for a live audio demonstration.

The goal is to provide an implementation sequence that an engineer could reproduce on many Cortex-M boards. It intentionally keeps the inference runtime independent of the HAL. Only the board-support layer should know whether incoming samples came from ADC+DMA, I$^2$S DMA, or a vendor audio peripheral.

![Signal path from acquisition to application decision.](figures/pipeline.png){width=4.65in}

## Stage 0: define one unambiguous data contract

Start by writing a small data contract and enforcing it in every module:

| Contract item | Lab choice | Reason |
|:--|:--|:--|
| Sample rate | 16,000 samples/s | Familiar low-rate speech front end |
| PCM | Signed 16-bit, mono | Compact and easy to validate |
| Frame length | 512 samples | 32-ms analysis window |
| Hop length | 320 samples | 20-ms new-feature period |
| FFT implementation | Real-input transform | Avoids redundant complex-input work |
| Feature representation | Explicit, versioned log-mel | Prevents training/firmware mismatch |
| Inference input | Quantized INT8 tensor | Fits common MCU inference kernels |
| Timestamp | Monotonic capture sample counter | Separates data age from task wakeup |
| Failure response | Invalid result, application action disabled | Makes stale-data behavior observable |

These are proposed lab values, not requirements of every keyword-spotting model. A deployed model may require a different frame length, preprocessing normalization, FFT size, or feature count. The model file and firmware must describe the *same* choices. It is an error to accept a neural network's plausible output as proof that its DSP preprocessing matches training.

## Stage 1: validate acquisition without ML

Configure the sampling timer or audio peripheral so that the transfer clock is derived from a known, documented clock tree. Use circular DMA into two halves of a statically allocated sample buffer; a completion ISR marks each half available. The interrupt path must be bounded: it updates an index or sequence number, records capture time, and signals a worker task. The worker task claims ownership of that half before DSP touches it.

A common conceptual pattern is

```text
DMA half-transfer -> publish block A -> wake acquisition worker
DMA full-transfer -> publish block B -> wake acquisition worker
acquisition worker -> copy to application ring -> release block
```

The two-half design is not automatically safe. If a slow worker still reads half A when DMA returns to that half, the producer can overwrite data. The buffer ownership policy must either guarantee processing is finished before reuse or detect, count, and safely discard overruns. A sequence counter is useful because a binary flag cannot distinguish one event from three events that occurred before the consumer woke.

Before adding inference, acquire many minutes of data while sweeping CPU load. Verify the sample count against a time reference, observe DMA callbacks, check for lost halves, and inspect a recorded waveform. Feed a known tone and verify that its FFT bin appears at the expected frequency. If the sample rate is in error by 1%, every frequency-dependent feature will be biased by 1% too.

## Stage 2: implement the overlapping frame builder

A streaming 512-sample frame updated every 320 samples requires 192 samples of overlap. One implementation keeps a 512-element frame buffer and, at each hop, moves the last 192 samples to the beginning before appending the new 320 samples. For a small fixed frame this is simple and testable. An alternative circular buffer eliminates the copy but makes contiguous FFT input generation and wrap handling more complex. Neither option is universally superior: choose based on measured copy cost and maintainability.

A frame always carries the monotonic sample index of its newest sample. If the consumer discovers a gap in the producer's sequence count, mark all affected frames invalid. Filling missing samples silently with previous audio may create plausible but misleading classifier outputs. A controlled dropout-handling policy should be part of the signal contract, not an undocumented convenience.

The frame producer need not run a full RTOS. A bare-metal superloop with a DMA ISR and a deterministic main-loop worker is valid if latency and responsiveness requirements are satisfied. The key property is clearly assigned buffer ownership, not the choice of scheduler API.

## Stage 3: reproduce preprocessing exactly

The front end applies a window $w[n]$, computes a transform $X[k]$, constructs a power or magnitude spectrum according to the model contract, applies a mel filter matrix $M_{j,k}$, and logs the result with a frozen floor $\epsilon>0$:

$$F_j=\log\left(\max\left(\epsilon,\sum_k M_{j,k}|X[k]|^2\right)\right).$$

If the model was trained with magnitude rather than power, using $|X[k]|^2$ changes its input distribution. Likewise, a $\log_{10}$ instead of a natural logarithm introduces a factor of $\ln 10$ before any normalization. An engineer should treat each preprocessing constant as part of the model artifact, not as a tunable firmware preference.

Test the DSP in isolation with synthetic vectors: silence, DC offset, bin-centered sine waves, a tone halfway between FFT bins, deterministic wideband noise, and near-full-scale impulses. Compare the target output to a high-precision offline implementation with explicitly documented tolerance. The floating-point CMSIS-DSP library is a sensible baseline when the target has an FPU; a quantized or fixed-point pipeline becomes attractive only when measurements justify its added numerical complexity.

## Stage 4: freeze quantization and graph metadata

The model artifact should include an integer version identifier, tensor shapes and layout, input/output scale and zero point, preprocessing revision, required operator list, and a content hash. Validate all these fields before allocating an arena or enabling the application action. Do not use an input tensor if its declared size exceeds its buffer or if the product of dimensions overflows `size_t`.

For an exported INT8 network, use the *actual* model's input scale and zero point when converting features:

$$q_{\rm feature}=\operatorname{clip}_{[-128,127]}\left(\operatorname{round}\left(\frac{F}{s_{\rm input}}\right)+z_{\rm input}\right).$$

An apparently harmless normalization change, such as subtracting a dataset mean twice, can destroy recognition accuracy. Golden test vectors should pin exact intermediate feature arrays as well as final classifier output. Those vectors are inexpensive to retain in a test fixture and dramatically reduce integration uncertainty.

## Stage 5: execute the model and report errors

Run inference from a worker context, not a DMA ISR. Separate the model invocation from detection policy. The inference result should contain at least:

```text
status / model revision / feature timestamp / invocation cycles
output logits or probabilities / validity / sequence number
```

This interface lets a supervisor suppress stale or invalid outputs without needing to understand operator internals. It also allows a developer to log a candidate detection without actuating a physical device. That separation is important for industrial products, where a false trigger might move machinery, open a valve, or cause an unsafe control transition.

A robust application may maintain separate states for stopped, learning/calibration, running, and faulted. In a faulted state, no classifier output should be authorized to cause an action. Recovery may require reinitializing the sensor stream and discarding partial windows. Simply zeroing a tensor is not necessarily safe: a learned network can assign high confidence to an all-zero input if the model is wrong or its distribution differs from training.

## Stage 6: a deterministic scheduling arrangement

![Recommended ownership and notification flow for MCU keyword spotting.](figures/scheduler.png){width=4.6in}

An RTOS is useful if acquisition, inference, telemetry, and other application work have different blocking behavior. The acquisition task typically has a higher effective urgency than telemetry, while the inference task must complete inside the time remaining after capture and DSP. A direct task notification can be cheaper than a queue when the event is only “new block ready,” but a notification alone does not carry a whole buffer ownership protocol. If multiple buffers can be pending, pair the notification with a protected ring of descriptors and an overrun counter.

With FreeRTOS, interrupt priority rules matter. An ISR calling `vTaskNotifyGiveFromISR()` must run at a priority allowed to call the kernel's FromISR API on that configured port. A more urgent ISR may deliberately avoid all kernel calls, but it then needs a different safe path for communicating with tasks. All the buffer transitions must be verified under back-to-back interrupt events, not just a leisurely debugger session.

Make the control/status task publish compact telemetry no faster than the transport can handle. At 115,200 bit/s UART, a nominal eight-N-one character costs ten serial bits, so a 100-byte line takes about 8.7 ms *on the wire*. Logging dozens of such lines each 20-ms hop is not benign background activity. A ring buffer plus low-priority draining task lets the real-time code drop optional telemetry rather than block and miss a deadline.

## Stage 7: instrument timing from first principles

Set up a counter or trace channel to record at least five boundaries: DMA block complete, frame ready, DSP complete, inference complete, and decision emitted. A timestamp associated with the original capture sample must survive every stage. Then compute

$$L_{\rm end\ to\ end}=t_{\rm decision}-t_{\rm newest\ sample}.$$

This excludes the age of the oldest samples in the analysis window; report both if the product requirement concerns the age of information used. A high-throughput implementation can still have poor worst-observed latency if its feature queue occasionally accumulates old windows. A queue depth of eight may conceal a transient overload while delivering stale detections long after the acoustic event.

Measure multiple system states: quiet idle, telemetry saturated, DMA continuous, nearby interrupt activity, and any low-power clock changes. Record compiler optimization and frequency. Treat p95 and observed maximum as measurements over the tested runs, not an unbounded WCET guarantee.

## Stage 8: acceptance criteria before a live demonstration

A release candidate should meet explicit gates:

- **Acquisition gate:** zero unexplained gaps over the chosen stress interval, or defined safe invalidation on each injected gap.
- **Feature gate:** documented error tolerance against the independent DSP reference for silence, tones, and representative recordings.
- **Numeric gate:** model output agrees with selected runtime reference on pinned tensors, including saturation and difficult integer endpoints.
- **Memory gate:** static image, arena, DMA, task stacks, and interrupts fit with an explicit engineering margin.
- **Timing gate:** measured response-time distributions and any formal/analytic bounds meet the stated product deadline under the specified conditions.
- **Detection gate:** representative false-alarm and miss rates, evaluated with the final deployed preprocessing and threshold logic.
- **Fault gate:** sensor dropout, stale model outputs, corrupted model metadata, and deadline faults all suppress unauthorized application actions.
- **Reproducibility gate:** exact model, toolchain, board revision, and test corpus identifiers accompany the result.

The author recommends saving these records alongside firmware release artifacts so a future engineer can distinguish a true regression from a toolchain, model, or laboratory change.

## From an STM32 lab to portable MCU code

Only a few modules need to change when porting from a Cortex-M4 to an ESP32 or RISC-V microcontroller: acquisition drivers, timer/interrupt primitives, memory-section placement, and the optimized kernel backend. Mathematical semantics, the reference test vectors, model schema validation, data freshness rules, and most C++ signal-processing code can remain platform independent. Keeping those boundaries visible is a major reason for using a host-testable design.

An application processor may be the better choice when a product needs large models, image pipelines, richer operating-system services, or significant dynamic workloads. That observation belongs in the system-selection discussion; it does not require the book to teach Cortex-A Linux deployment. This book's central subject is the discipline of producing reliable learned functions on the microcontroller itself.

## Chapter design review

The finished lab is not “neural network runs on a board.” It is a reproducible chain from timed sensor acquisition through testable numerical operations to bounded, fault-aware application behavior. Only when the entire chain is measured on the intended hardware do performance, power, and reliability claims become meaningful. The code supplied with the book is a starting point for the numerical layer, while the procedure here specifies the hardware integration work a real engineering project still needs to perform.

**Exercises**

1. At 16 kHz with a 512-point frame and 320-point hop, calculate the overlap in samples and milliseconds. What is the worst frame backlog after three delayed hops if each frame is queued instead of replaced?
2. Design a two-half DMA ownership state machine that cannot silently overwrite an unprocessed block. State how a fault is reported.
3. Compare an ISR+superloop implementation with a FreeRTOS implementation for the same pipeline. Which specific product requirements justify the added scheduler?
4. Propose a benchmark CSV schema that includes sensor timestamp, capture-to-feature latency, inference cycles, classifier scores, and fault counters, without causing blocking UART work in the critical path.
