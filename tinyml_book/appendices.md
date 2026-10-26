\appendix


# Essential Formulas

**Affine INT8 value:** $r\approx s(q-z)$; **inverse:** $q=\operatorname{clamp}(\operatorname{round}(r/s)+z)$.

**Quantized dot product:** $A=\sum_i(q_{x,i}-z_x)(q_{w,i}-z_w)+b_q$.

**Output requantization:** $q_y=\operatorname{clamp}(\operatorname{round}(A s_xs_w/s_y)+z_y)$.

**Dense MACs:** $mn$ for $m$ inputs and $n$ outputs.

**Convolution MACs:** $NH_oW_oC_oK_hK_wC_i$.

**Depthwise+pointwise MACs:** $NH_oW_o(K_hK_wC_i+C_iC_o)$ for depthwise multiplier one.

**FFT:** $X[k]=\sum_{n=0}^{N-1}x[n]e^{-j2\pi kn/N}$.

**Sliding feature context:** $T_{\rm context}=(N+(R-1)H)/f_s$.

**Response-time iteration (simplified):** $R_i^{n+1}=C_i+B_i+\sum_{j\in hp(i)}\lceil R_i^n/T_j\rceil C_j$.

**Energy:** $E=\int P(t)dt$; **MAC utilization:** $U\approx C/T$ (subject to task-model assumptions).

# Glossary

**Activation:** a tensor produced by an operator (usually transient). **Arena:** a statically reserved region used for tensor/runtime storage. **CMSIS-DSP:** Arm's optimized numerical/DSP kernel library. **CMSIS-NN:** Arm's optimized neural-network kernels for Cortex-M. **Depthwise convolution:** per-input-channel spatial filtering. **DSP front end:** deterministic transform from sensor samples to model input features. **INT8:** signed 8-bit integer inference representation. **Log-mel:** logarithm of mel-spaced spectral-band energies. **MAC:** multiply-accumulate operation. **NHWC:** batch-height-width-channel tensor order. **Per-channel quantization:** distinct weight scales per output channel. **PTQ:** post-training quantization. **QAT:** quantization-aware training. **Requantization:** converting an integer accumulator into another tensor's scale and zero point. **SPSC:** single-producer/single-consumer. **WCET:** worst-case execution time, which ordinary profiling does not prove.

# MCU Bring-Up Checklist

1. Record exact MCU, board revision, pin/clock/power settings and toolchain SHA/version.
2. Prove peripheral capture timing and sample continuity before inference.
3. Verify DMA buffers are accessible and aligned; identify overwrite/overflow policy.
4. Build the smallest graph and compare its output to a host reference vector.
5. Validate tensor arena capacity and actual use; capture map-file SRAM/flash.
6. Measure warm and cold initialization separately.
7. Measure inference during representative sensor/RTOS load.
8. Confirm watchdog behavior on stalled worker, bad model, and missing sensor.
9. Verify outputs are rejected after expiry or stream generation reset.
10. Archive release hashes, recordings, telemetry and test procedure.

# References and Further Reading

The sources below are normative only for their own documented library versions; exact APIs, operator versions and compiler behavior must be checked against the revision used for a build.

1. Arm. *CMSIS-NN Software Library and quantization specification.* <https://arm-software.github.io/CMSIS-NN/latest/index.html>.
2. Arm. *CMSIS-DSP: Embedded Compute Library.* <https://github.com/ARM-software/CMSIS-DSP>.
3. TensorFlow Lite Micro. *Source, core concepts and documentation.* <https://github.com/tensorflow/tflite-micro>.
4. TensorFlow Lite Micro. *Memory Management.* <https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/docs/memory_management.md>.
5. TensorFlow Lite Micro. *MicroInterpreter interface.* <https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/micro_interpreter.h>.
6. J. Lavrenz / NetKit Labs. *KWSKit: Real-time Keyword Spotting on Cortex-M4F.* <https://github.com/NetKit-Labs/kwskit>, particularly the hardware validation and DSP notes. Observations quoted in this book are from the repository's recorded 2026 Zephyr session, not from a new test performed for this manuscript.
7. J. Lavrenz / NetKit Labs. *NetKit: Neural Network Kit.* <https://github.com/NetKit-Labs/netkit>, particularly `docs/STATUS.md` and the STM32 peer benchmark logs. Quoted results are from the repository, not newly collected for this book.
8. J. Lavrenz / NetKit Labs. *MemKit: C/C++ Memory-Efficient Containers.* <https://github.com/NetKit-Labs/memkit>.
9. S. Haykin. *Adaptive Filter Theory.* An engineering reference for filtering and numerical methods.
10. A. V. Oppenheim and R. Schafer. *Discrete-Time Signal Processing.* A foundational reference for DFT, windowing, and spectrum estimation.

11. Arm. *CMSIS-NN 8.0.0 release notes*, 21 August 2026. <https://github.com/ARM-software/CMSIS-NN/releases/tag/v8.0.0>. Experimental floating-point kernels and existing integer contracts must not be conflated.
12. Arm. *CMSIS-NN Convolution Functions.* <https://arm-software.github.io/CMSIS-NN/latest/group__NNConv.html>. Review the selected wrapper's scratch requirements.
13. Arm. *CMSIS-DSP Real FFT Functions.* <https://arm-software.github.io/CMSIS-DSP/latest/group__RealFFT.html>. Check signatures and buffer requirements for the selected architecture.
14. NetKit Labs. *KWSKit Zephyr hardware validation.* <https://github.com/NetKit-Labs/kwskit/blob/main/docs/validation.md>. Records the normal-run timing and buffer measurements cited in this book.
15. NetKit Labs. *NetKit MNIST-CNN STM32 peer comparison.* <https://github.com/NetKit-Labs/netkit/blob/main/docs/MNIST_CNN.md>. Reports accelerated and reference-kernel comparisons separately.

# Validation and Publication Status

**Expanded manuscript, 26 October 2026.** The companion C++20 reference functions in `examples/int8_reference/` compile and pass deterministic and seeded randomized host tests, including a sanitizer run in the preparation environment. Chapter 4 preserves a separate pedagogical interface skeleton; Chapter 12 documents a completed educational implementation with an explicit rational requantization contract. It is *not* claimed bit-exact against CMSIS-NN or TFLM, is not a trained model, and is not a complete firmware product. Hardware measurements in Chapters 1, 8 and 10 come from the author's independently recorded public-project sessions; no new MCU hardware timing, energy, or acoustic measurements have been obtained for this manuscript. Independent technical review, thorough code auditing, copyediting, target-board testing, and a physical print proof should precede commercial publication.

# Worked Arithmetic Examples

**F.1 Per-channel quantization.** For input $x=(1,2)$, weights $w_0=(3,-1)$ and $w_1=(-2,4)$, zero bias, and zero input point, the accumulators are $A_0=1$ and $A_1=6$. For channel multipliers $M_0=1/2$ and $M_1=1/4$, and output zero point $z_y=-3$, nearest ties-away gives outputs $q_0=-2$ and $q_1=-1$. This example shows that per-channel multipliers matter even when the input scale is shared.

**F.2 Zero-point-correct convolution.** For $x=(1,2,3,4)$, kernel $(1,2,1)$, and one sample of zero padding at both ends, the output is $(4,8,12,11)$. Shifting the stored input to $(6,7,8,9)$ and assigning zero point $z_x=5$ leaves the centered signal unchanged. The padding code must also be 5, not a zero byte.

**F.3 Scheduling utilization.** A model invoked every 20 ms with execution demand 5 ms contributes nominal task utilization 25%. That is not itself proof of a 20-ms response bound; blocking, interrupt time, other tasks and release jitter must be included.

**F.4 Memory placement.** A 60-KiB constant model mapped into program flash does not automatically consume 60 KiB of SRAM. A loader that copies the same weights into a mutable tensor does. Check the link map and startup image; a C++ storage qualifier alone does not prove where bytes reside.

# Build and Test Provenance

Build the reference kernels from the book root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

`std::span` presents non-owning buffer views; it does not allocate. The reference execution routines do not use dynamic allocation, but the surrounding application still needs to validate model metadata, tensor lifetimes, and output freshness. Host execution of golden vectors does not certify embedded timing, safety, or device compatibility.

Rebuild the document using `./build.sh` with Pandoc and XeLaTeX. The current PDF uses a 6-by-9-inch page. A print-on-demand provider may require separate checks for trim, margins, fonts, cover layout, metadata, and proofing. A technically complete *manuscript* is not identical to a professionally printed, validated product.
