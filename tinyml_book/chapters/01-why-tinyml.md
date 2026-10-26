# The Engineering Case for TinyML

## A learned function is still embedded software

A product need not recognize a thousand object classes for machine learning to be useful. Consider a remote vibration monitor that must identify an abnormal operating pattern, a wake-word interface that must listen without an Internet connection, or a current-sensing controller that must classify operating conditions. In each case the computational problem is local, the inputs arrive continuously, and a decision must meet the same resource, timing, power, safety, and test requirements as the rest of the firmware.

The deployed function may be expressed as

$$y_k = f_{\theta}(\phi(x_{k-L+1:k})),$$

where $x$ is a physical measurement stream, $\phi$ is deterministic preprocessing, $\theta$ is a fixed set of learned parameters, and $f_\theta$ is an inference graph. The firmware also needs postprocessing and a stateful decision policy:

$$a_k = g(y_k,a_{k-1},t_k,\mathcal{F}_k),$$

where $\mathcal{F}_k$ includes sensor validity, dropped data, freshness, and system faults. The action $a_k$ is the product behavior. It is *not* simply the network's largest probability.

A classifier is therefore only one stage of an embedded signal chain:

> Sensor → acquisition → buffering → preprocessing → inference → confidence/temporal policy → actuator or event.

This boundary matters organizationally. The model developer may produce $\theta$, but the firmware team must integrate and verify the *entire* chain. The pipeline must be debuggable with no connection to the training environment.

## When a neural network is preferable

Start with the classical alternative. Many embedded problems admit a hand-designed estimator, filter, threshold, or state machine. A learned model is justified when the input-output relationship is difficult to express economically as explicit rules and a representative labeled dataset exists. It is not justified merely because it can be made to run on the MCU.

Compare candidates using a design objective, not an accuracy-only metric:

$$J = w_e E_{\rm error} + w_t T_{\rm inference} + w_r R_{\rm RAM}
       + w_f F_{\rm flash} + w_p P_{\rm energy} + w_m M_{\rm maintenance}.$$

The weights encode product priorities, and a hard constraint cannot be traded away with a good average score. If $T_{\rm deadline}=10\,\mathrm{ms}$, an algorithm taking $12\,\mathrm{ms}$ is inadmissible even if it has excellent recognition accuracy. Similarly, a speech classifier with a 5% false-trigger rate may be unusable no matter how small its model is.

**Example: a bearing-health monitor.** Compare (A) RMS and kurtosis thresholds, (B) a small feature-based classifier, and (C) a compact convolutional network on a spectrogram. The correct comparison is made against the same dataset split, load/speed operating range, false-positive requirement, and sensor mounting variation. Option A may win if fault modes separate cleanly. Option B may preserve interpretability and use less compute. Option C may generalize to more complex signatures but requires lifecycle management of data and weights.

**When not to use TinyML:** deterministic calibration transforms; small finite-state logic; tasks requiring hard analytical guarantees that the network cannot supply; products without representative deployment data; or systems where updates, traceability, and testing costs exceed the added functionality.

## Why the MCU is a distinctive target

A microcontroller typically executes code from flash, has a bounded on-chip SRAM budget, provides timer/interrupt peripherals, and may have no operating system or only a small RTOS. Contrast this with an application processor whose caches, virtual memory, filesystem, and large DRAM make quite different architectural compromises possible.

The MCU introduces four coupled constraints:

1. **Persistent storage:** weights, graph metadata, lookup tables, and firmware occupy flash.
2. **Working storage:** feature windows, activations, scratch buffers, stacks, DMA rings, and RTOS objects compete for SRAM.
3. **Time:** a chain of operators must complete before the next meaningful decision or buffer deadline.
4. **Energy and heat:** average and peak current matter, especially in duty-cycled systems.

The important conceptual shift is to budget the *complete* firmware image, not a standalone neural-network model. A nominal 40 KiB arena is not a 40 KiB application: it exists alongside audio buffers, call stacks, drivers, diagnostics, and other real-time tasks.

## Four feasibility calculations

### Arithmetic budget

For a fully connected layer with $m$ inputs and $n$ outputs, the nominal multiply-accumulate count is $mn$. An MCU executing an effective $C$ MAC/s would require approximately

$$T \approx \frac{mn}{C}+T_{\rm dispatch}+T_{\rm memory},$$

which is a starting estimate, not a measurement: memory layout, instruction selection, saturation, and cache/bus behavior affect $C$. A complete convolutional network must be evaluated layer by layer rather than extrapolating from a single operator.

### Storage budget

For a graph with weights $W_i$, metadata $M$, non-persistent activations $A$, persistent runtime state $P$, application buffers $B$, and stacks $S$,

$$F_{\rm required} \gtrsim F_{\rm application}+\sum_i |W_i|+M,$$

$$R_{\rm required} \gtrsim A+P+B+S+R_{\rm drivers}+R_{\rm margin}.$$

The activation term should reflect lifetime-based reuse, not a naive sum of all intermediate tensors; Chapter 5 derives this precisely.

### Sampling versus decision rate

For microphone sampling at $f_s=16\,000\,\mathrm{Hz}$, an audio hop of $H=256$ samples arrives every

$$T_H=H/f_s = 16\,\mathrm{ms}.$$

An inference that takes $55\,\mathrm{ms}$ cannot run on every 16-ms hop on the same core without a strategy for decimation, pipelining, overlap, or backlog. But it *may* run every eight hops ($128$ ms) if acquisition can preempt processing and the end-to-end budget holds. The high-priority producer must not be blocked by the consumer.

### Latency budget

For a windowed detector, the total response has more than one component:

$$T_{\rm response}=T_{\rm observation}+T_{\rm queuing}+T_{\rm preprocess}
+T_{\rm infer}+T_{\rm decision}+T_{\rm actuation}.$$

Increasing model speed cannot reduce the observation window. A nearly one-second context window may dominate the first decision after startup even if inference is fast.

## A real Cortex-M case study: continuous keyword spotting

The author's open-source KWSKit repository describes a Zephyr application on an nRF52840 Cortex-M4F at 64 MHz, with 16-kHz audio, CMSIS-DSP log-mel features, and an INT8 inference model. The recorded Zephyr session reports an average inference time of about 54.994 ms, 65,536 bytes of reserved arena with 12,500 bytes reported used, and an 8,192-sample audio ring whose recorded high-water mark was 768 samples. These are *specific test observations* from that repository, not generalized performance claims or WCET bounds [KWSKit, 2026].

This case illustrates a broader engineering pattern: sensor acquisition has higher urgency than occasional inference, a fixed ring decouples production and consumption, and detection events carry timestamps so a late result cannot blindly trigger action. The details are revisited in Chapters 6, 7, and 10.

## Chapter design review

Before selecting an MCU inference architecture, write down: the false-positive/false-negative costs; operating-domain coverage of the dataset; sample and decision periods; maximum accepted time-to-action; worst-case resource ceilings; OTA/model-upgrade policy; safe behavior on invalid or stale data; and the non-neural baseline. A proposed TinyML design without these items remains a demonstration, not an embedded product architecture.

**Exercises**

1. For $f_s=16$ kHz and $H=160$, calculate the hop period. Can a 38-ms inference run each hop on a single non-overlapping worker?
2. Given 128 KiB SRAM, reserve 24 KiB for stacks/RTOS, 16 KiB for DMA/audio, and 8 KiB margin. What is the maximum combined activation plus persistent model allocation?
3. Propose a classical alternative to a neural wake-word classifier, including the experiment needed to compare them fairly.
