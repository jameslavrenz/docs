# Real-Time Firmware Integration

## Throughput and deadlines are different properties

An inference algorithm can process one second of audio in less than one second and still fail an embedded deadline. Throughput says how much work is completed per unit time; real-time correctness says the work is completed *when required*. For a pipeline with acquisition period $T_a$, feature hop period $T_h$, and inference decision period $T_d$, first write a temporal contract for each stage.

A periodic job $\tau_i$ has execution time $C_i$, relative deadline $D_i$, period $T_i$, and release jitter $J_i$. In the simplest fixed-priority, preemptive, single-core model without blocking, a sufficient schedulability analysis uses response-time iteration

$$R_i^{(n+1)}=C_i+\sum_{j\in hp(i)}\left\lceil\frac{R_i^{(n)}}{T_j}\right\rceil C_j,$$

where $hp(i)$ contains higher-priority periodic tasks. A real firmware system may require added blocking, ISR interference, DMA contention, release jitter, and library critical sections:

$$R_i^{(n+1)}=C_i+B_i+I_i(R_i^{(n)})
+\sum_{j\in hp(i)}\left\lceil\frac{R_i^{(n)}+J_j}{T_j}\right\rceil C_j.$$

These equations assume a specified task model and bounded execution times. They are not automatically applicable to all driver threads or all RTOS priority schemes. Measure and model the actual implementation.

## A two-rate architecture

Treat capture and inference as separate rates. An input peripheral may deliver blocks at a fixed high rate. A DSP worker processes every hop. Inference may run every $K$ hops to bound CPU occupancy:

$$T_d=KT_h,\qquad U_{\rm infer}\approx\frac{C_{\rm infer}}{KT_h}.$$

Suppose $T_h=16$ ms, $K=8$, and a *measured typical* inference time is 55 ms. The nominal utilization of that worker is $55/128\approx43\%$. This is not yet a schedulability guarantee: the correct $C_{\rm infer}$ for hard analysis is a justified worst-case bound, and preemptions, DSP work, queues, and other tasks consume CPU time.

**Do not assign the most CPU-intensive task the highest priority automatically.** The long inference calculation may be allowed to yield to an audio producer whose bounded-time deadlines are much tighter.

## FreeRTOS and Zephyr choices

The scheduling principle is portable, but APIs and integration details differ. In FreeRTOS, a task may block on a direct-to-task notification or a queue. On Zephyr, a semaphore or message queue may be the natural primitive. Both require an explicit plan for producer rate, lost notifications, buffer lifetime, and overrun behavior. Avoid making an ISR do memory-intensive work unless the latency analysis demands and justifies it.

A task notification is not the same as a queue of data: a counting notification can report work availability but usually does not preserve the payload of each interrupt. For audio, the data belongs in a separate ring whose write and read indices are protected by the correct memory-ordering rules. If the producer and consumer run in different interrupt/task contexts, `volatile` alone does not establish portable synchronization semantics in C++.

## Single-producer/single-consumer ring

With a power-of-two capacity $N$, a ring can map a monotonic logical index $i$ to array index $i\bmod N$ by a mask. The producer must publish the data *before* publishing the corresponding write index. The consumer must observe the write index *before* reading newly published data. The simplest safe implementation on a bare-metal or RTOS MCU may use short critical sections for index snapshots; an atomic implementation must use release/acquire ordering and respect the architecture's atomicity guarantees.

If write index is $w$ and read index is $r$ (both monotonically increasing unsigned counters, with explicit wrap assumptions), the occupancy is $w-r$. It must remain $\le N$. Define an overflow policy: reject newest block; drop oldest and mark a discontinuity; or stop acquisition and fault. The correct policy depends on product requirements. A speech detector should normally reset partial detection evidence after losing samples, since old and new feature rows cannot be treated as one continuous temporal context.

## A timestamp is a safety-relevant field

An inference output is time-dependent evidence, not timeless truth. Each result should include a timestamp or sequence number and a generation number that changes when acquisition continuity is lost. A consumer may reject a result if

$$t_{\rm now}-t_{\rm result}>T_{\rm max\_age}$$

or if its generation does not match the current stream. The generation check also prevents an old confidence score from triggering a delayed actuator action after a stream reset.

A queue may have room for eight results, but the consumer can still be stalled for longer than their validity interval. The correct policy can be to drop stale results rather than process every historical score. This favors *freshness* over completeness for many interactive embedded devices.

## Temporal confidence filtering

A model probability $p_k$ often needs stabilization:

$$s_k=\alpha p_k+(1-\alpha)s_{k-1},\qquad 0<\alpha\le1.$$

A detector may require $M$ consecutive threshold crossings, apply hysteresis with an activation level $T_{\rm on}$ and release level $T_{\rm off}$, and enforce a refractory/cooldown interval. This logic is not trivial UI polish; it determines false activations, repeated events, and end-user behavior.

Consider two firmware builds with identical model weights but different $\alpha$, timeouts, and stream reset behavior. They are *different detectors*. Therefore detector state and configuration must be versioned and included in validation, not omitted from the model description. (Here $\alpha$ denotes the EMA coefficient.)

## Avoid a false WCET claim

A loop measured 55.054 ms maximum during one session has an observed maximum of 55.054 ms **within that test**. It does not have a proven WCET of 55.054 ms. Interrupt patterns, temperature, clock modes, DMA/bus contention, driver paths, and input-dependent branches may change execution time. Hard timing claims need an appropriate bounded analysis or exhaustive argument, which is substantially more demanding than profiling.

Use measured timing distributions for characterization and conservatively bounded budgets for design. Record instrumentation overhead and measurement start/stop boundaries. For a Cortex-M DWT cycle counter (where implemented and enabled), $T=\Delta\mathrm{CYCCNT}/f_{\rm core}$ only when the clock and counter behavior are known. The counter wraps, and other executing work can contribute to the elapsed interval if profiling does not isolate tasks.

## Watchdogs, faults, and actions

A safe design should specify what happens if: sensor stops; DMA overflows; preprocessing produces a NaN; tensor allocation fails; a kernel returns an error; inference misses deadlines; an output score expires; or the model hash differs from the approved version. Detection should not continue from an invalid time series after a buffer overrun.

The inference worker must not be the sole watchdog of itself. A lower-frequency supervisor can monitor progress, but urgent physical safe-state actions may need interrupt-level or dedicated hardware mechanisms. Neural inference should not be used to bypass safety interlocks that require independent deterministic controls.

## Budgeting worksheet

| Pipeline element | Period or trigger | Execution bound | Failure policy |
|---|---|---|---|
| PDM/ADC acquisition | peripheral event | measured + bounded | count/drop/fault |
| Ring publication | each block | short constant-time | reject + generation bump |
| DSP feature extraction | each hop | bounded by frame size | discard on backlog |
| Neural inference | every $K$ hops | measured; bound TBD | skip stale run |
| Detection policy | result event | small bounded | reject stale output |
| Telemetry | best effort | no control-path waits | drop diagnostics |

The discipline is to fill every cell with an actual system requirement or measurement and resist substituting a demonstration's average for a release criterion.

**Exercises**

1. Calculate the nominal utilization of a 35-ms inference invoked every 80 ms. Then explain why it does not prove deadline satisfaction.
2. Design a minimal event record whose fields detect stale scores, lost samples, and stream resets.
3. Identify a priority inversion scenario involving telemetry and a shared log mutex; propose a bounded remedy.
