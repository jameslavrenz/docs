# A Complete Design Walkthrough: Keyword Spotting

## State the requirement before selecting the graph

The example product is a single-keyword voice interface on a Cortex-M-class MCU. It samples at 16 kHz and must generate an action only for confirmed, fresh keyword detections. It should continue capturing while inference runs, expose loss and timing counters, and avoid blocking time-critical acquisition on logging or user-interface work.

These are **design targets** for the walkthrough, not validated release requirements for a particular consumer product. Real deployments need defined environmental conditions, false-activation ceilings, acoustic coverage, EMC/power requirements, and failure-response policies.

## Select a feature contract

Use frames $N=512$, hop $H=256$, 20 mel bands, and 61 feature rows. The feature tensor is $61\times20$ INT8 (1,220 bytes). The first decision can only occur once the 992-ms context is filled. Thereafter, invoke at a nominal eight-hop interval of 128 ms, subject to backlog and freshness policies.

The feature extractor maintains the following invariants:

1. hop sequence numbers increase monotonically during a continuous stream;
2. a feature row is published only after a complete frame is available;
3. a ring overrun bumps the stream generation and invalidates the old context;
4. the quantized feature bytes match stored model input parameters;
5. a DSP error cannot cause stale rows to be treated as new evidence.

A ring capacity of 8,192 samples represents 512 ms of 16-kHz audio storage in the absence of other limits. This capacity is not equivalent to half a second of tolerated *inference pause* under all circumstances; DSP and queue backlogs may already be consuming the available margin.

## Plan the software components

A cohesive module map is:

```text
Clock/peripheral setup
   |
   +--> microphone capture callback --> SPSC PCM ring
                                      |
                                      v
                               DSP feature worker
                                      |
                                      v
                            sliding feature context
                                      |
                                      v
                               INT8 inference engine
                                      |
                                      v
                           timestamped score message
                                      |
                                      v
                              detection state machine
                                      |
                                      v
                                  LED / event

Telemetry runs separately and may discard diagnostic output.
```

The detector never obtains a raw pointer into the inference engine's temporary output tensor if that pointer could be invalidated by the next run. Instead copy the small score payload (and timestamps/generation) into a bounded message. This avoids a lifetime coupling between inference and the lower-priority consumer.

## Assign ownership and synchronization

A sound fixed-memory design gives exactly one writer to each principal state object:

| State | Owner | Readers | Synchronization |
|---|---|---|---|
| PCM ring write index | capture context | DSP worker | acquire/release or short critical section |
| PCM ring read index | DSP worker | capture context | acquire/release or short critical section |
| Sliding features | DSP worker | inference in same worker | no concurrent access |
| Interpreter arena | inference worker | no external writer | single owner |
| Score message queue | worker produces | detector consumes | bounded RTOS queue |
| Detection latch | detector | telemetry snapshot | short protected copy |

The table is a design pattern, not an exact description of every KWSKit source module. The repository is useful for exploring one concrete implementation and its timing/fault validation [KWSKit, 2026].

## Detection semantics

A temporal filter receives $(t_k,p_k,seq_k,gen_k)$. It rejects an event that is older than $T_{\rm age}$, whose stream generation differs from the current generation, or whose sequence continuity policy has been violated. A qualifying fresh event updates an EMA and a debounce counter. An action occurs only when the high threshold and confirmation count are satisfied outside cooldown. A lower release threshold re-arms the detector.

Formally, if $v_k$ is the validity predicate,

$$s_k=\begin{cases}\alpha p_k+(1-\alpha)s_{k-1},&v_k,\\
0,&\neg v_k\text{ with reset policy},\end{cases}$$

and the actuator trigger is a guard over $(s_k,p_k,\text{count}_k,\text{latch}_k,t_k)$. A guard should be stated as a state-machine transition table and tested over adversarial event sequences, not only over recorded ordinary speech.

## Resource worksheet

For the chosen feature geometry: 1,220 bytes of INT8 feature context, plus 16,384 bytes for an 8,192-sample INT16 PCM ring, plus 512-sample framing/FFT workspaces, model ROM, tensor arena, and task stacks. The 16,384-byte ring figure assumes one two-byte PCM sample per entry. These are arithmetic calculations from selected design parameters, not whole-program measurements.

A model's arena size should be measured using its actual operator resolver and optimized kernels. Scratch requirements can differ between a reference convolution and CMSIS-NN. Reserve memory headroom for RTOS and peripheral work before declaring a model feasible.

## Timing worksheet

Let the DSP worker handle each 16-ms hop; inference is requested every 128 ms. A design budget might initially require:

$$C_{\rm DSP,per\ hop}+C_{\rm producer\ interference}<16\,\text{ms},$$

$$C_{\rm infer}+C_{\rm DSP,interference}+C_{\rm other}<128\,\text{ms},$$

but these inequalities are only necessary approximations under a single-core schedule. Actual response-time analysis includes priority, release jitter, blocking, and backlog. An inference run of ~55 ms measured in a reference session is encouraging but does not replace that analysis.

## Instrumentation

Expose counters and clocks with units:

```text
capture_samples_total       (samples)
ring_overflow_total         (blocks or samples, explicitly defined)
feature_rows_total          (rows)
feature_processing_max_us   (microseconds observed)
inference_elapsed_max_us    (microseconds observed)
inference_error_total       (count)
score_expired_total         (count)
detection_actions_total     (count)
stream_generation           (monotonic sequence)
```

Counters must be saturating or have defined wrap behavior; atomicity must be appropriate to the writer/reader contexts. Do not print from the highest urgency ISR to make diagnostics convenient. Capture raw logs for repeatable offline analysis.

## What the existing measured result proves—and does not

In its documented Zephyr hardware session, KWSKit demonstrated live audio capture, inference, and keyword-controlled LED actions on a 64-MHz Cortex-M4F. It reported a normal inference average of 54.994 ms, an observed maximum of 55.054 ms, and no reported audio loss during the cited normal capture [KWSKit validation, 2026]. Those observations support the feasibility of its *particular implementation under that test*. They do not establish production acoustic accuracy, all-environment robustness, a hard WCET bound, or suitability for a different MCU clock/memory configuration.

This evidence distinction is fundamental. Publishing what was actually tested makes the result more credible, not less.

## Milestone plan for a new STM32 implementation

**Milestone 1:** host-only operator parity and full model vector parity. **Milestone 2:** STM32 image links and boots with the model embedded; verify flash/RAM map. **Milestone 3:** standalone inference on canned features with cycle counts and correct outputs. **Milestone 4:** microphone/ADC capture validated with ring counters. **Milestone 5:** feature parity from captured sensor audio. **Milestone 6:** full streaming detector, faults, and timing under load. **Milestone 7:** field-condition accuracy and extended soak validation.

Do not call a project 'hardware-validated end-to-end TinyML' after Milestone 3. Testing an embedded inference operator and validating a real sensor application are separate achievements.

## An engineer's acceptance checklist

A strong design review can answer yes, with evidence, to all of the following:

- Is the quantized model byte-for-byte versioned and linked to the training manifest?
- Is the entire preprocessing transform specified, including FFT/log conventions?
- Are tensor quantization and operator versions compatible with deployed kernels?
- Do all allocations have bounded lifetime and known memory placement?
- Can acquisition meet its deadline while inference runs?
- Are sensor gaps, late results, and restarts handled explicitly?
- Are runtime and detector faults reported and routed to an appropriate safe state?
- Do on-device tests compare exact or tolerance-bounded numerical outputs?
- Are measured flash, RAM, cycles and observed latency distributions archived?
- Are field accuracy and false-activation measurements separated from training accuracy?

If those questions can be answered with repository evidence, a small neural network has been elevated from a demonstration to a well-engineered embedded software component.

**Exercises**

1. Describe a fault injection that confirms a delayed score cannot trigger an action after a stream reset.
2. List three changes that require retraining or at least renewed model validation even if C++ operator code is untouched.
3. Write a requirements-to-test traceability matrix for a wake-word device that includes acquisition continuity, stale score rejection, and a false-activation ceiling.
