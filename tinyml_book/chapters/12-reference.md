# A Tested INT8 Reference: From Algebra to C++20

## The contract before the implementation

The companion directory `examples/int8_reference/` contains complete C++20 source for three narrowly scoped primitives: deterministic fixed-point requantization, a row-major dense layer, and a one-dimensional cross-correlation. `CMakeLists.txt` builds the library, a console demonstration, and a host test program. This deliberately modest implementation is **original educational reference software**. It is not a FlatBuffer interpreter, not a trained keyword-spotting network, and not a drop-in replacement for CMSIS-NN or TFLM.

Such restraint is useful. A correct dense or convolution kernel has an independently testable algebraic contract. An interpreter also needs model compatibility, operator versions, tensor metadata, memory planning, and multiple backend interfaces; introducing all those concerns at once makes it difficult to diagnose an arithmetic error. Build a small trusted layer first, then expand the graph only as real requirements demand.

The reference implementation uses signed INT8 activations, symmetric signed INT8 weights (weight zero point is zero), signed INT32 biases, wide INT64 intermediate accumulation, and output zero-point adjustment followed by activation-domain clipping. Its multiplication scale is an explicit rational number $n/2^k$ with $n>0$ and $0\le k\le31$. This is intentionally **not** the requantization representation used by CMSIS-NN. Matching an established runtime byte-for-byte requires adopting that runtime's exact multiplier normalization and rounding contract.

## The fixed-point rounding derivation

For an accumulator $A\in\mathbb Z$ and positive multiplier $M=n/2^k$, define the mathematical reference

$$r=\operatorname{round}_{\rm ties\ away}(AM).$$

Let $P=An$ be evaluated in signed 64-bit arithmetic and $D=2^k$. If $P\ge0$, the desired result is

$$r=\left\lfloor\frac{P+D/2}{D}\right\rfloor$$

for $D\ge2$; for $D=1$ the expression reduces to $r=P$. For negative $P$, apply the same formula to $|P|$ and restore the negative sign. This implements nearest-integer rounding with exact half-integer ties away from zero. The domain restrictions matter. Given signed INT32 $A$ and positive signed INT32 $n$, $|P|\le2^{31}(2^{31}-1)<2^{62}$; therefore the product and its magnitude fit signed INT64. Adding $D/2\le2^{30}$ remains safe. When $k=0$, the divisor is one, and the rounding correction is zero.

A common but incorrect shortcut is `(acc * multiplier) >> shift` with a 32-bit intermediate. It can overflow before the shift. It also relies on implementation-defined signed right-shift behavior for negative integers in older C++ language versions, and does not implement the declared tie rule. The reference code uses positive-magnitude division to make all these semantics explicit.

The result after rounding may still exceed INT32, so `requantize()` **saturates** to the INT32 range before adding the output zero point. A model builder should reject scales that yield unacceptable saturation, rather than relying on saturation to conceal a bad scale. Since the outer output is INT8, the final activation range is constrained to $[-128,127]$.

## Worked dense-layer vector

Consider a four-element input

$$x=[1,2,3,4]^T$$

and three output channels with row-major weights

$$W=\begin{bmatrix}1&1&1&1\\2&0&-2&0\\-1&1&-1&1\end{bmatrix},\quad b=[0,0,2]^T.$$

Let both zero points be zero and use per-output multipliers $(1/2,1/2,1)$. The three accumulators are

$$A_0=10,\qquad A_1=2-6=-4,\qquad A_2=-1+2-3+4+2=4.$$

Requantization gives

$$q_y=[5,-2,4]^T.$$

The runnable example prints `dense output = [5, -2, 4]`. The host test independently asserts these exact values. If these expected results change after an optimization, the optimization is wrong for this documented contract or the contract itself was intentionally revised. Neither should happen silently.

### Zero point as an offset, not a cosmetic field

Suppose an input tensor stores $[10,11]$ with $z_x=10$ and weights $[1,2]$. The centered input is $[0,1]$, and its dot product is 2, **not** $10+22=32$. The test verifies that output. The same rule governs padding: a sample outside the valid input region corresponds to real zero, so the stored padding code is $z_x$, whose centered value is zero. Treating the padding byte as numerical zero introduces a systematic border error whenever $z_x\ne0$.

## A one-dimensional convolution with explicit layout

The reference 1-D convolution accepts input shape $[T,C_{\rm in}]$, weight shape $[C_{\rm out},K,C_{\rm in}]$, and output shape $[T_{\rm out},C_{\rm out}]$. For kernel width $K$, left and right pads $P_l,P_r$, and stride $S$, the output time extent is

$$T_{\rm out}=\left\lfloor\frac{T+P_l+P_r-K}{S}\right\rfloor+1$$

provided the padded extent is at least $K$. The operator computes *cross-correlation*, meaning the learned kernel is not reversed. For an output channel $o$ and output time $t$,

$$A_{t,o}=b_o+\sum_{k=0}^{K-1}\sum_{c=0}^{C_{\rm in}-1} \tilde x_{tS+k-P_l,c}w_{o,k,c},$$

where $\tilde x$ is the centered input inside valid bounds and zero outside. This is the convention used by contemporary machine-learning libraries even when their operator is named `Conv1D` or `Conv2D`.

A golden case uses $x=[1,2,3,4]$, kernel $[1,2,1]$, stride one, and one sample of padding on both ends. The expected outputs are

$$y=[4,8,12,11].$$

At the first position, the receptive field is $[0,1,2]$ and the sum is $0+2+2=4$. At the last, it is $[3,4,0]$ and the sum is $3+8=11$. The test repeats the entire calculation after adding 5 to every input and setting $z_x=5$; the output must remain unchanged. This proves something concrete about the padding semantics.

For a true 2-D convolution, extend the indexing to $[H,W,C_{\rm in}]$ and $[C_{\rm out},K_H,K_W,C_{\rm in}]$, with separate height and width strides, dilation, and padding. The same dot-product and requantization rules apply. A real kernel needs careful shape validation and scratch-memory planning before replacing this educational 1-D core.

## Shape safety and arithmetic safety are distinct

Checking `weights.size()==outputs*inputs` using an unchecked multiplication is unsafe if the dimension product overflows the host integer type. The reference uses a checked product helper, returning a non-success status on overflow or mismatched extents. Quantization parameters and output activation limits are validated as well. This is especially important when model metadata comes from a file or firmware update rather than from audited compile-time constants.

Within each dot product, the implementation uses a signed 64-bit intermediate and checks whether the final accumulator fits signed INT32 before narrowing. A host test constructs a 140,000-term dot product whose mathematical value exceeds INT32 and confirms that `accumulator_overflow` is reported. This illustrates *rejection rather than silent wraparound*. The compiler's type widths and application memory limits still matter; the educational example does not claim a universal proof of 64-bit accumulation safety for arrays so large that the sum itself could exceed INT64. An embedded product should constrain tensor dimensions before invocation.

These routines deliberately do not support buffer aliasing beyond the documented contracts. Aliasing mutable output with the read-only input would change the meaning of a streaming operator. Where in-place operations are desired, the developer must prove their traversal order and lifetimes are safe. `std::span` gives a view, not ownership and not an anti-aliasing guarantee.

## Host test process and provenance

Build and execute in a checked development configuration:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/tinyml_demo
```

To use runtime sanitizers on a compatible Clang/GCC host:

```sh
cmake -S . -B build-san -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

The reference tests include exact half-tie rounding in both signs; invalid scale parameters; a seeded randomized comparison against a higher-precision oracle; shape mismatch rejection; nonzero input zero points; zero-point-correct convolution padding; and accumulator overflow rejection. These tests were run successfully on the development host during manuscript preparation. They do **not** establish that the example is production-grade, thoroughly fuzzed, ARM cross-compiled, compatible with CMSIS-NN bit-exact arithmetic, or safe under all hypothetical enormous tensor sizes.

`CMakeLists.txt` builds the library with `-Wall -Wextra -Wpedantic -Werror` for supporting GNU/Clang toolchains. A firmware port should also compile with the Arm cross-compiler and the target's actual ABI and optimization flags, and run the same golden vectors on the microcontroller before attempting performance claims.

## Why not implement the entire neural-network runtime here?

A complete production runtime needs a model format with schema validation, a deterministic operator registry, multidimensional tensor metadata, static arena planning, scratch contracts, quantization versioning, extensible dispatch, and a supported set of fused operations. More source code is not automatically better pedagogy. A small, completed kernel implementation establishes the *inner numerical contract* in a way a ten-thousand-line interpreter would obscure.

Consider a three-layer inference graph with a 1-D convolution, a rectified activation, and a dense classifier. Each layer can use the primitives in the reference project, but the surrounding model still needs a legitimate training/export process. The test vectors shipped with the book are synthetic fixed weights, not a trained audio recognizer. Never use their output as evidence of classification accuracy.

An application can instead integrate a mature runtime such as TFLM and its CMSIS-NN backend. That shifts the engineering focus from implementing all kernels to correctly exporting the graph, selecting operator registrations, allocating the tensor arena, integrating input DSP, handling errors, benchmarking the executable, and preserving regression vectors across compiler updates. The reference code remains a valuable oracle for a small subset of arithmetic decisions.

## Cross-runtime parity is a separate test objective

Two mathematically equivalent quantized kernels may disagree by one integer count because they implement different tie rounding, high-multiply saturation, or shift semantics. That discrepancy should not be dismissed without analysis, but neither does it automatically establish a bug. To claim TFLM/CMSIS-NN parity, freeze exact operator versions and apply those kernels' documented quantization contracts. Compare outputs for positive and negative half-way cases, extreme accumulators, all relevant zero points, and per-channel scale sets. If performance code must reproduce an existing runtime exactly, the original runtime's reference implementation—not this educational one—is the bit-exact oracle.

This distinction also affects claims made about the author's NetKit inference engine. A measured speedup is meaningful only for a specific hardware/graph/kernel combination with identical requested semantics and quality constraints; the example in this chapter does not assert a new, independent benchmark against any external runtime.

## A realistic next build increment

An engineer extending this reference could implement a small static graph described by a compile-time array of nodes. Each node would declare input and output tensor extents, scratch requirements, and an operator version. The initializer would reject unsupported types and compute tensor lifetimes before reserving an aligned arena. The execution loop would execute a deterministic schedule without allocation or I/O, returning a status plus timing counters. Each operator could then be dispatched to a portable implementation, a CMSIS-NN backend, or another validated MCU backend.

The important sequence is **correctness, memory accounting, schedulability, optimization, then deployment validation**. Reversing that order often produces an impressive demo that is difficult to maintain or audit.

## Chapter design review

The example distinguishes four requirements frequently conflated in discussions of TinyML: (1) a mathematical function, (2) a numerical fixed-point contract, (3) a memory-safe executable, and (4) a real-time deployed system. The companion repository exercises the first three in a limited, explicit domain. Only hardware deployment and full system verification can establish the fourth.

**Exercises**

1. Replace a per-tensor multiplier with a separate rational multiplier for each output channel and construct a test where the outputs differ by at least two counts.
2. Extend the 1-D reference to support dilation and derive the resulting output-length equation. What dimensions must be checked before the execution loop begins?
3. Estimate the worst accumulator magnitude for a convolution with kernel $3\times3$, 64 input channels, $|q_x-z_x|\le255$, and $|q_w|\le127$. Is an INT32 accumulator bound adequate before adding bias?
4. Propose a compile-time static graph with memory reuse between two non-overlapping intermediate tensors. Prove there is no read-after-reuse violation.
