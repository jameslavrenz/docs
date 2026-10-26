# Optimizing Kernels on Cortex-M

## Establish a correct baseline first

Optimization begins by freezing a reference implementation, a fixed model input corpus, and numerical acceptance criteria. A faster kernel that disagrees with the quantization specification has not improved the product. Benchmark operator and whole-graph latency separately: an operator may become faster while scratch allocation, layout conversion, or dispatch changes eliminate the system-level gain.

Arm Cortex-M family members vary substantially. Cortex-M4 provides DSP instructions and can include a single-precision FPU; later Cortex-M variants may offer different SIMD or M-profile vector extensions. Do not assume code optimized for a vector-capable core is efficient—or even compilable—on an STM32F446's Cortex-M4F. CMSIS-NN provides target-aware integer kernels, but exact supported operators and APIs depend on the library revision [CMSIS-NN].

## Why data movement dominates unexpectedly

A matrix kernel performs arithmetic but must also fetch inputs and weights. Define arithmetic intensity

$$I=\frac{\text{useful MACs}}{\text{bytes transferred}},$$

under a stated memory hierarchy and transfer accounting. A naive convolution reloads the same input neighborhood across output channels; a tiled implementation increases reuse by holding a small block in registers or scratch. This may use more SRAM but reduce transfer and instruction overhead.

The latency can be decomposed conceptually:

$$T\approx T_{\rm kernel\ setup}+T_{\rm arithmetic}+T_{\rm loads/stores}
+T_{\rm requantize}+T_{\rm dispatch}.$$

These terms overlap on a pipelined processor, so they should not be treated as independently measurable additive constants in all circumstances. The decomposition is useful for hypothesis generation and microbenchmarks.

## Packing and channel blocking

In INT8 convolution, a kernel may pack multiple signed 8-bit elements into registers, issue packed arithmetic instructions, and accumulate into wider integer registers. Channel blocking can reduce loop overhead and improve weight reuse. The packing order must match the model's memory layout, sign extension, and the hardware's intrinsic argument conventions.

An im2col transformation converts receptive-field patches into matrix rows, making a convolution look like matrix multiplication. The advantage is reuse of an optimized matrix kernel; the cost is a scratch buffer and extra copies. On a small SRAM device, a partial im2col or direct convolution can be superior. The best choice is dependent on kernel dimensions, channel count, available scratch, and the core.

## Benchmark matrix

Measure at least the following dimensions:

| Variable | Why it matters |
|---|---|
| Input/weight/output shape | channels, stride, and kernel geometry determine locality |
| Quantization variant | per-tensor vs per-channel changes requantization |
| Core and clock | cycles and frequency must both be reported |
| Optimization level | compiler vectorization and inlining differ |
| Link-time optimization | may remove generic dispatch but alter layout |
| Scratch allocation | performance and peak RAM are coupled |
| Cache/memory placement | MCU families have different SRAM bus characteristics |
| Data corpus | path-dependent activations, saturation and branches |

A result should provide median, tail/observed maximum, number of repetitions, warmup policy, measurement overhead, binary size, and numerical parity status. A ratio such as $T_{\rm baseline}/T_{\rm optimized}$ should identify both baselines. '10x faster' without a peer configuration is not a meaningful engineering claim.

## Interpreting a real measured comparison

The author's NetKit repository records STM32 NUCLEO-F446RE at 180 MHz, using INT8 peer comparisons against TensorFlow Lite Micro and microTVM. For one recorded MNIST CNN graph, NetKit's CMSIS-NN configuration reports 95.3 ms against TFLM at 95.5 ms, whereas a reference-kernel path reports 336.2 ms against a much slower TFLM reference path at 2593.5 ms. These are *different kernel configurations*. The strong reference-to-reference gap must not be presented as a production CMSIS-NN-to-CMSIS-NN advantage [NetKit STATUS, 2026].

The engineering lesson is that the backend often determines performance more strongly than the surrounding interpreter framework. A benchmark with equivalent optimized kernels may be nearly tied even if its reference implementations differ dramatically.

## A repeatable board benchmark

An automated benchmark harness should:

1. identify MCU, core frequency, compiler and git commit;
2. initialize exactly the same model and test input for both runtimes;
3. warm caches, branch paths or runtime initialization according to a specified policy;
4. call the model repeatedly with monotonic cycle timestamps;
5. compare outputs elementwise and report disagreements;
6. report runtime allocation/arena capacity and link-size information;
7. preserve raw logs and build flags in the repository.

If the system includes a free-running RTOS, measure whether timer interrupts and DMA workloads remain active in the benchmark. Disabling them makes a cleaner kernel microbenchmark but may understate end-to-end interference in the deployed system. Both measurements can be useful if labeled correctly.

## Power: energy per inference is not clock cycles alone

For instantaneous power $P(t)=V(t)I(t)$,

$$E_{\rm inference}=\int_{t_0}^{t_1}P(t)\,dt.$$

A 30% latency reduction does not necessarily imply 30% lower energy: the optimized kernel may alter instantaneous current or enable earlier sleep. A product may be driven by *energy per useful detection* rather than energy per isolated inference. For example, feature extraction continues between model runs, and sensor power may dominate the compute budget.

Meaningful power tests control supply, clock configuration, device sleep state, UART logging, measurement bandwidth, and model cadence. Measure with suitable equipment, or state clearly that power remains uncharacterized.

## Algorithmic optimizations before assembly

The largest improvement may come from architecture changes rather than intrinsics:

- choose a smaller feature tensor or reduce inference cadence while retaining detection sensitivity;
- replace standard convolutions with suitable depthwise/pointwise layers;
- remove avoidable graph reshapes or quantize/dequantize boundaries;
- fuse bias, activation, and requantization;
- precompute constants and pack flash-resident weights;
- eliminate unnecessary copies between input features and interpreter tensors;
- exploit arena liveness and static graph scheduling.

Every such modification can affect model accuracy or streaming response latency. Perform regression testing after each major change.

**Exercises**

1. Explain why speeding up a convolution by 2x might improve a complete network by only 10%.
2. Design an A/B test that distinguishes CMSIS-NN kernel performance from interpreter overhead.
3. Estimate energy for a constant 35 mW compute load during a 60-ms inference. What information is needed to claim device energy savings?
