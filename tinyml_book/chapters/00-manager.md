# TinyML: An Engineering Manager's Briefing


![The decision gate ties statistical improvement to explicit embedded budgets.](figures/decision-new.png){width=4.45in}

## The product decision, not the buzzword

TinyML means deploying a learned function in a microcontroller's ordinary firmware execution environment. It is worth evaluating when rules, thresholds, and conventional estimators are not sufficiently robust across real operating conditions, and when a representative labeled data set can be collected and maintained. A neural network does not remove the need for requirements, observability, reliability, or configuration management: it adds a second controlled artifact, the model, to the software product.

A manager should ask for **three alternatives** before funding a TinyML implementation: a classical DSP/threshold baseline; a compact feature-based statistical model; and the proposed neural network. Compare all three against the same operating envelope and held-out data, using false alarms per operating hour, missed events, energy, RAM, flash, full-system latency, and maintenance cost rather than classification accuracy alone.

## Engineering scope and staffing

The embedded team owns sensor acquisition, timing, memory layout, model compatibility, fault behavior, firmware updates, and post-deployment measurement. Data/model specialists, whether internal or external, own dataset quality, training, and statistical evaluation. The handoff is a *versioned executable contract*: sampling rate, preprocessing coefficients, tensor layout, quantization parameters, model hash, operator set, acceptance vectors, and permissible input domain.

A useful first deliverable is not a demonstration video. It is a four-page feasibility record: model-versus-baseline accuracy by operating subgroup; measured on-target CPU/energy budgets; memory map and stack/arena high-water marks; and a test report for stale/corrupt/missing input behavior. Only after those gates should integration complexity increase.

## Typical failure modes

- **Dataset shortcut learning:** the model learns microphone, machine, or recording-session identity instead of the intended signal.
- **Train/deploy skew:** windowing, log base, integer scaling, or sensor gain differs between Python and firmware.
- **Unbounded background load:** profiling covers a standalone kernel but not DMA, interrupts, queues, and telemetry.
- **Unsafe confidence semantics:** an argmax label is treated as permission to actuate; stale outputs are not rejected.
- **Undocumented retraining dependencies:** a preprocessing or sensor revision silently invalidates the model.

## Decision gate

Approve the architecture only when a documented baseline underperforms, an evaluation set represents foreseeable deployment conditions, MCU memory/latency margins are measured, and ownership of model updates is assigned. Reject or defer it if none of these conditions can be established. For safety-relevant actions, the learned classifier should normally inform a separately verified deterministic safety policy rather than act as the sole authority.

**For an interview discussion:** the most interesting engineering question is rarely "Can this MCU run a neural network?" It is "Can this product justify a learned function and operate it predictably for years?"
