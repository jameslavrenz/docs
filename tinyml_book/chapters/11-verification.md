# Verification, Benchmarking, and Deployment Evidence

## A performance number is a claim with a scope

A timing result is not an intrinsic property of a neural network. It belongs to the entire measurement configuration: network parameters, kernel library and commit, CPU architecture, compile flags, linker placement, clock tree, wait states, cache state if present, buffer alignment, power conditions, interrupt activity, and measurement boundary. A useful report specifies those details. A sentence such as “the model takes 8 ms” is incomplete; the engineer must ask whether that was kernel execution alone, a full invocation, or the entire path from microphone sample to decision. The engineer must also ask how many trials were measured, what input distribution was used, and whether the result includes an occasional interrupt or DMA contention.

A book should never promote an illustrative timing calculation to an observed result. This edition deliberately contains no fabricated board measurements. The independently compiled reference kernel in the companion code establishes algorithmic and software-test evidence on the host. The benchmarking protocol in this chapter is intended for use when firmware is deployed to a known target board. That distinction is itself part of rigorous embedded engineering.

**Evidence levels.** In decreasing order of confidence for a particular claim, one might distinguish (a) production-like hardware observations covering the specified operating envelope, (b) repeatable controlled board benchmarks, (c) instruction-set simulation or cycle-accurate models with validated fidelity, (d) host numerical tests, and (e) analytical estimates. The order is not absolute for every property: formal analysis can establish certain memory-safety or deadline bounds that testing cannot. Nonetheless, a fast host unit test never establishes a measured Cortex-M execution time.

## From requirements to evidence

A useful verification system begins with explicit, falsifiable requirements rather than a notebook of ad hoc tests. Consider a keyword spotter integrated into an industrial monitor. Its requirements might include:

- An input rate of $16\,000$ samples/s, with monotonic timestamps and explicit loss detection.
- A feature hop of $H=320$ samples, generating a fresh feature vector every $20$ ms.
- A stated maximum age of a successful classification at the moment a detection is raised.
- Flash and SRAM use below hardware allocation budgets, including interrupt, DMA, and RTOS memory.
- A bounded response to stale input, corrupted model metadata, sensor faults, numerical failures, and missed deadlines.
- Quantified false-trigger and missed-trigger rates on a held-out *field-representative* corpus.

The requirement is incomplete until it specifies its verification method. A sample-rate requirement is checked with timer/divider configuration, scope/logic-analyzer observations, and long-running sample counts; a no-heap-in-inference requirement is checked by static code review, a link-map audit, runtime allocation instrumentation, and a deliberately constrained allocator in tests. Each method tests a different proposition.

| Requirement | Suggested verification | Important exclusion |
|:--|:--|:--|
| Input continuity | Sequence numbers and DMA half/full transfer counters | A good-looking waveform alone |
| INT8 numerical semantics | Fixed golden tensors, intermediate-layer parity | Only checking final argmax |
| SRAM peak | Link map, stack watermarks, arena high-water and DMA reservations | Summing model weights |
| Invocation time | DWT cycles and monotonic elapsed time | Desktop wall time |
| System latency | Trace sensor timestamp through decision | Kernel cycles only |
| Energy per decision | Integrated supply current and rail voltage | CPU load percentage |
| Detection accuracy | Held-out operating-condition corpus | Training accuracy |
| Safe failure | Inject and verify disabled actions | Merely logging an error |

## The timing hierarchy

Define $T_s$ as the sampling interval, $T_h$ as the feature-hop interval, $T_f$ as preprocessing time, $T_i$ as inference time, and $T_d$ as the decision-stage execution time. A new output cannot become available until enough new input exists to advance the feature window. If the application performs processing synchronously on each hop, the processing demand per hop is

$$C_{\mathrm{hop}}=C_{\mathrm{capture\ bookkeeping}}+C_f+C_i+C_d.$$

It is *necessary*, but not by itself sufficient, that $C_{\mathrm{hop}} < T_h$: preemption, contention, and blocking can increase response time. If $C_i=7$ ms and $T_h=20$ ms in a hypothetical example, the nominal utilization contribution is $0.35$ for one such job per hop. This is a calculated illustration only. It does not imply that the real project has $7$ ms inference latency.

The age of data matters more than the processing time alone. For a frame of length $N$ ending at $t_e$, a detector output at $t_o$ uses data whose earliest sample is $N/f_s$ seconds old. An informative bound therefore separates **capture-window latency**, **queueing delay**, **computation**, and **decision filtering**. At $N=512$, $f_s=16$ kHz, and a new hop every 20 ms, a frame spans 32 ms. The time since its newest sample is substantially smaller than 32 ms if processing begins promptly; treating the full frame length as identical to compute latency obscures what has been measured.

A system that averages over $K$ consecutive decisions adds temporal evidence accumulation. In a simple detector requiring $K$ successive positive hops, the decision can be delayed by approximately $(K-1)T_h$ beyond the first positive hop, plus queueing and processing. That delay is an algorithmic property, not an RTOS scheduling bug.

## Fixed-priority response-time analysis

For a preemptive fixed-priority uniprocessor system, a conventional response-time recurrence for task $i$ is

$$R_i^{(n+1)}=C_i+B_i+\sum_{j\in hp(i)}\left\lceil\frac{R_i^{(n)}}{T_j}\right\rceil C_j,$$

where $C_i$ is an execution-time bound, $B_i$ is a bounded blocking term, and $hp(i)$ is the set of strictly higher-priority tasks. Starting with $R_i^{(0)}=C_i+B_i$, iterate to convergence or until the result exceeds the specified deadline. The ceiling counts potentially interfering releases. This idealized equation assumes independent periodic/sporadic tasks under the stated priority assignment and does not automatically account for release jitter, interrupt execution, DMA stalls, cache effects, self-suspension, or non-preemptible sections. If such effects are material, the model must be extended.

**Illustrative calculation.** Suppose a 20-ms inference task has $C_i=4$ ms, blocking $B_i=0.4$ ms, and a higher-priority capture task with $C_h=0.2$ ms and period $T_h=2$ ms. Then $R^{(0)}=4.4$ ms, $R^{(1)}=4.4+\lceil4.4/2\rceil(0.2)=5.0$ ms, and $R^{(2)}=5.0$ ms. On those assumptions it meets a 20-ms deadline. The calculation is not a substitute for establishing credible $C_i$, $B_i$, and hardware-interference bounds.

Scheduling also affects which operations belong in interrupts. The DMA ISR should publish a completed buffer, update a short counter or timestamp, and wake a task. It should not perform FFTs, invoke a model, allocate memory, or emit formatted telemetry. High interrupt execution time can make a high-priority task late even if task-level CPU utilization looks low.

## Measuring Cortex-M cycles

On Cortex-M processors with a usable DWT cycle counter, a minimal timing boundary looks like:

```cpp
// Pseudocode: initialize DWT according to device and debug policy.
const auto start = DWT->CYCCNT;
model.invoke();
const auto elapsed = DWT->CYCCNT - start;  // unsigned wrap arithmetic
```

For a 32-bit counter clocked at $f_{\mathrm{CPU}}$, the wrap period is $2^{32}/f_{\mathrm{CPU}}$. At 180 MHz this is approximately 23.86 s. An elapsed interval shorter than one wrap can be obtained through unsigned subtraction, but a silent multiple-wrap interval cannot. If the core clock changes dynamically, convert cycles using the clock rate that actually applied to the measured interval. Enable and availability of DWT vary by core/device and debug configuration; a firmware port must check these properties rather than assuming all Cortex-M designs behave identically.

A robust measurement captures both the bare invocation time and the end-to-end time. If instrumenting a system with FreeRTOS, keep instrumentation outside critical paths as much as possible: record compact events into a bounded buffer and dump them asynchronously. UART logging inside a timed region distorts the result and may change the worst-case schedule. Use a GPIO edge and scope trace to validate timing independently when the measurement matters.

## Statistics and worst-case claims

A median communicates typical execution behavior; a high percentile helps describe variability. Neither is a worst-case execution-time (WCET) proof. If $n$ observations yield a maximum $C_{\max}$, report **observed maximum over $n$ runs**. Do not label it WCET unless a valid timing-analysis or bounded-hardware argument supports that claim.

Record the number of trials, warm-up procedure, clock, optimization flags, environment, sensor activity, interrupts, whether DMA was active, and whether tested inputs changed the operator paths. If input-dependent sparsity shortcuts exist, random test data may select paths different from the field workload. A static shape does not always mean constant execution time.

For a benchmark table, use a schema rather than inventing impressive numbers:

| Field | Captured value |
|:--|:--|
| Board / MCU / revision | To be measured |
| Core / clock / flash wait states | To be measured |
| Source + model SHA-256 | To be recorded |
| Toolchain, optimization, LTO | To be recorded |
| Peak tensor arena | To be measured |
| Flash model and total image | To be measured |
| Median / p95 / observed max cycles | To be measured |
| Sample count / measurement boundary | To be recorded |
| Temperature and motor/sensor workload | To be recorded |

The reader can reproduce the table for one fully specified hardware configuration. A public performance claim should link that table to the exact firmware and model revisions. Different microcontrollers, even at the same frequency, have very different memory systems and DSP/MVE capabilities.

## SRAM verification as a runtime activity

The linker map establishes static placement, but total RAM use can also include task stacks, temporary DMA buffers, heap allocations during initialization, and peripheral memory visible through the address space. A fixed tensor arena should report its high-water mark; each RTOS task should report stack high-water; fault and ISR stacks need their own bounds or conservative proof. On a small MCU, the difference between a 32-KiB and a 40-KiB arena could determine whether a firmware image fits at all.

Guard words help detect *some* overflows after they occur; they do not establish absence of overflow. Stack-pattern watermarks can miss a path that hasn't run yet. A better strategy combines static worst-case frame information where available, diverse stress paths, fault injection, and a reserved engineering margin. If the RTOS allocates some control blocks from its own heap, that heap must be part of the budget even when the application code performs no dynamic allocation during inference.

## Energy per inference

Energy is the time integral of supplied electrical power:

$$E_{\mathrm{inf}}=\int_{t_0}^{t_1} V(t)I(t)\,dt.$$

When voltage is approximately constant, $E\approx V\bar I\Delta t$. This does not imply that reducing execution time always reduces total device energy: waking peripherals, changing clocks, putting RAM to sleep, and reactivating a radio can dominate the budget. A realistic battery-powered device should measure energy over a representative *duty cycle*, including acquisition, inference, sleep, decision, and communication.

A power meter that averages over a second cannot resolve a short inference pulse on its own. Depending on the desired accuracy, use a shunt and oscilloscope/current monitor with known bandwidth, sample-rate calibration, and a measurement boundary established by a GPIO signal. Separate the microcontroller rail from sensor and driver rails when interpreting component-specific claims. For a complete product, report system-level energy as well as core-only energy.

## A reproducible optimization experiment

Suppose a reference convolution is replaced by a CMSIS-NN kernel. Make one change at a time and preserve a golden output for every layer. Measure correctness and three resource dimensions: cycles, flash, and RAM. An optimized implementation can be faster and still be unsuitable because temporary scratch memory exceeds the available SRAM. Likewise, a compressed INT4 weight format may reduce flash without reducing end-to-end latency if decompression overhead dominates on the selected core.

**Experiment protocol:**

1. Freeze model, test corpus, platform clock, power policy, and compiler revision.
2. Run golden vectors through reference and optimized variants, recording intermediate differences or explicit bit-exact parity.
3. Rebuild from a clean tree with the same flags and inspect the link map.
4. Record timing distributions under quiet load and realistic interrupt/DMA load.
5. Repeat measurements after reordering variants to expose thermal or battery drift.
6. Record measurement uncertainty and explain any observed regressions.
7. Save a machine-readable report with commit IDs and environmental metadata.

A reproducible negative result is useful. If a hand-written unrolled kernel loses to the compiler-generated scalar loop on a Cortex-M4, publish the conditions and keep the simpler kernel. The goal is not to prove that an optimization was clever; the goal is to select the correct engineering tradeoff.

## Verification through deliberately hostile inputs

A happy-path recognition demonstration exercises only a fraction of the real system. Fault injection should include buffer backlog, skipped DMA completion, encoder or sensor dropout (as applicable), corrupted model metadata, saturated features, underflow/overflow, missing synchronization, and timebase discontinuity. Each fault should have an expected state transition and a bounded response. Particularly important is the **stale-data** failure: the inference engine may continue to return valid-looking outputs from an old feature window if freshness is not represented explicitly.

For an audio application, an accepted detection should carry the latest input timestamp, the model ID, the feature revision, and a validity flag. If a feature window is stale or partly corrupted, suppress the application action even if the classifier score is high. A continuous stream with no detected DMA errors does not prove a valid stream unless counter wrap and buffer ownership have been analyzed.

## Chapter design review

Treat a result as a tuple of **claim, configuration, evidence, uncertainty, and provenance**. Every high-level engineering decision should be traceable to such a tuple. Successful host unit tests establish that the example code implements its stated contracts on the tested host configurations; they do not establish embedded deadline compliance, energy consumption, end-user accuracy, or resistance to adversarial inputs. Those require separate evidence.

**Exercises**

1. Derive a response-time bound for a 25-ms inference task with $C=6$ ms, $B=0.7$ ms, and higher-priority tasks $(C,T)=(0.3,2)$ ms and $(0.8,10)$ ms. State the assumptions.
2. Compare two hypothetical implementations: 7 ms at 12 mA and 12 ms at 6 mA, each at 3.3 V. Which uses less active energy? What other energy is missing?
3. Design a fault-injection test that establishes bounded rejection of stale audio, including queue ownership, timestamps, and the signal used to measure stop latency.
