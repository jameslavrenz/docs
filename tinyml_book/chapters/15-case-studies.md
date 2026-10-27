# Measured MCU Case Studies and Engineering Interpretation


![The system-level signal pipeline explains the required timing boundaries.](figures/pipeline-new.png){width=4.45in}

## Evidence discipline

The numbers below come from published project records in the author's open-source repositories. They were **not newly gathered for this edition**. The value is in interpreting what the records do and do not establish. A comparison is only fair when input tensors, operator versions, optimization choices, measurement boundary, clock, and backend are accounted for.

## KWSKit: real-time audio on Cortex-M4F

KWSKit's recorded Zephyr session used an nRF52840 Cortex-M4F at 64 MHz, 16-kHz streaming input, DSP feature computation and INT8 inference. Its normal-run telemetry reported **54.994 ms average and 55.054 ms observed maximum inference time**, a DSP average/maximum of **2.217/5.249 ms** per consumed hop, processing-step maximum **57.342 ms**, and audio-ring high-water **768 of 8192 samples**. The original validation note identifies the exact test session, its recording scope, and no observed losses in that session.

The key system insight is that a 55-ms inference invocation cannot be repeated on every 16-ms data hop on a single core without overload. KWSKit instead runs model inference at a lower cadence with capture separated from processing; its recorded schedule attempts inference every eight new feature rows (128 ms), subject to readiness and backlog checks. The high-water statistic is a measured property of that run, not a proof for every combination of interrupt load and input environment.

**Source:** [KWSKit validation](https://github.com/NetKit-Labs/kwskit/blob/main/docs/validation.md) and [inference notes](https://github.com/NetKit-Labs/kwskit/blob/main/docs/tinyml-inference.md). A rigorous reader should inspect the run logs and exact firmware revision before using these values as an external benchmark.

## NetKit: kernel choice on STM32F446RE

NetKit documents the same MNIST-CNN workload on an STM32 NUCLEO-F446RE with different inference backends. With CMSIS-NN enabled, **NetKit 95.3 ms** and **TFLM 95.5 ms** are essentially at parity; microTVM AOT is **112.3 ms** in the recorded comparison. With reference kernels, NetKit's recorded time is **336.2 ms**, while TFLM reference is **2593.5 ms** and microTVM reference **343.0 ms**. These reference/accelerated figures must not be mixed into a single speedup statement.

An important interpretation is that *vendor-optimized kernels may dominate throughput*. If two interpreters invoke comparable CMSIS-NN routines, extremely different framework headlines may largely disappear. A runtime's real differentiation may then be arena management, graph handling, portability, binary size, or integration simplicity, not faster arithmetic. Conversely, the reference path is highly sensitive to implementation strategy and compiler behavior.

**Source:** [NetKit STM32 peer comparison](https://github.com/NetKit-Labs/netkit/blob/main/docs/MNIST_CNN.md) and the [NetKit platform status](https://github.com/NetKit-Labs/netkit/blob/main/docs/STATUS.md). These are author-operated project comparisons, not independently replicated results.

## Suggested figures for a defensible public performance comparison

For every case, provide an image or trace of the timing boundaries, a compiler/version inventory, sample count, model and firmware hashes, clock tree, interrupt state, input vectors, arena high-water, and a power measurement where available. The book's companion `benchmarks/measurement_template.csv` deliberately leaves new measurement values blank. Use the actual hardware records rather than inserting estimates into a publication graphic.

## A correct hardware experiment changes the conclusion

It would be tempting to call 95.3 versus 95.5 ms a 0.2-ms framework victory. Without sufficient trial counts, jitter distributions, and identical build conditions, that is not a meaningful claim. The more robust observation is close accelerated parity. Similarly, a 55.054-ms *observed* maximum is not a worst-case execution-time guarantee. Good engineering communication preserves these distinctions even when stronger numbers would be more marketable.
