# Appendices

## A. Essential formulas

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

## B. Quick glossary

**Activation:** a tensor produced by an operator (usually transient). **Arena:** a statically reserved region used for tensor/runtime storage. **CMSIS-DSP:** Arm's optimized numerical/DSP kernel library. **CMSIS-NN:** Arm's optimized neural-network kernels for Cortex-M. **Depthwise convolution:** per-input-channel spatial filtering. **DSP front end:** deterministic transform from sensor samples to model input features. **INT8:** signed 8-bit integer inference representation. **Log-mel:** logarithm of mel-spaced spectral-band energies. **MAC:** multiply-accumulate operation. **NHWC:** batch-height-width-channel tensor order. **Per-channel quantization:** distinct weight scales per output channel. **PTQ:** post-training quantization. **QAT:** quantization-aware training. **Requantization:** converting an integer accumulator into another tensor's scale and zero point. **SPSC:** single-producer/single-consumer. **WCET:** worst-case execution time, which ordinary profiling does not prove.

## C. Board bring-up checklist

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

## D. Reading and references

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

## E. Publication and validation status

**Draft dated 26 October 2026.** This is an original technical manuscript. The author has not claimed to run the chapter's illustrative C++ skeletons as a complete firmware build. Code requiring `requantize(...)` has an explicitly unspecified implementation to be filled and tested against a chosen runtime contract; it is intentionally not presented as a finished library. Hardware metrics appearing in Chapters 1, 8, and 10 are externally documented observations from the author's named public repositories. Revalidate repository data, figures, citation links, source-code samples, numerical rounding rules, and all print proofs before final paperback publication.
