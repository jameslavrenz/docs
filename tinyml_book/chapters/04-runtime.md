# Building a Small Inference Runtime in C++

## From an equation to an executable contract

A production interpreter needs a model loader, tensor descriptors, operator registry, memory plan, and dispatch loop. To understand the essential mechanics, start smaller: a fixed sequence of operators with compiled-in weights, statically reserved storage, no heap use in the inference path, and straightforward error reporting. This section develops **reference code** intended to be adapted and tested, not a drop-in production inference library.

We will implement a simple dense network and then specify a quantized convolution. The objective is *correctness and ownership clarity*. Acceleration will come later.

## An explicit tensor view

A tensor view does not own its storage. It describes an existing buffer whose lifetime must exceed every use of the view.

```cpp
#include <cstddef>
#include <cstdint>
#include <span>

struct Quantization {
  float scale;           // positive and finite
  std::int32_t zero;     // inside the element range
};

struct TensorI8 {
  std::span<std::int8_t> data;
  std::span<const std::size_t> shape;
  Quantization quant;
};
```

The `span` object is only a pointer plus size; it does not allocate. However, neither `span` nor `shape` verifies that the product of dimensions equals the data extent. Construction must validate this, including integer overflow in dimension multiplication. Firmware should generally move full validation to model initialization, leaving the hot path with stable validated shapes.

For fixed graph dimensions, compile-time arrays may be preferable:

```cpp
struct FeatureVector {
  static constexpr std::size_t kCount = 20;
  alignas(4) std::int8_t values[kCount]{};
};
```

This communicates lifetime, alignment, and storage cost directly. Do not confuse C++ `constexpr` with an obligation to write an entire generic template framework. The smallest useful abstraction is often the best embedded abstraction.

## Ownership, status, and failure

Model weights can reside in a `const` flash-backed object; mutable activations reside in SRAM. A function accepting `std::span<const int8_t>` cannot modify weights. Firmware should report failures without dynamically allocated exception objects if exceptions are disabled:

```cpp
enum class Status : std::uint8_t {
  ok, shape_mismatch, bad_quantization,
  arena_exhausted, unsupported_operator,
  numerical_failure
};
```

Explicit status values allow an upstream supervisor to distinguish a bad model image from a runtime sensor fault. In release firmware, assertions should enforce programmer invariants only if the policy for assertion failure is clearly defined. Unexpected model data and recoverable I/O failures should not rely on debug-only assertions.

## A simple floating-point reference layer

Before adding quantization, implement an unoptimized baseline whose mathematics is obvious:

```cpp
bool dense_f32(std::span<const float> x,
               std::span<const float> weights_row_major,
               std::span<const float> bias,
               std::span<float> y) {
  const std::size_t m = x.size();
  const std::size_t n = y.size();
  if (bias.size() != n ||
      (m != 0 && n > weights_row_major.size() / m) ||
      weights_row_major.size() != m * n) return false;
  for (std::size_t j = 0; j < n; ++j) {
    float acc = bias[j];
    for (std::size_t i = 0; i < m; ++i)
      acc += weights_row_major[j * m + i] * x[i];
    y[j] = acc;
  }
  return true;
}
```

The precondition guard above should be tightened for production, particularly at zero-size boundaries and overflow. A more robust implementation uses a helper `checked_mul(m,n)` and rejects invalid model dimensions *once at initialization*. Keep the reference kernel small and readable; its role is to establish the expected result for optimization.

For a deterministic host test, compare against a double-precision calculation for small random tensors. Define absolute and relative error tolerances with care:

$$|y-\hat y|\le\epsilon_{\rm abs}+\epsilon_{\rm rel}|y|.$$

Near zero, relative error by itself is a poor criterion. Floating-point accumulation order may change under compiler optimization or vectorization; distinguish mathematically acceptable differences from uninitialized memory and indexing faults.

## Quantized dense reference kernel

A signed INT8 dense layer uses an INT32 accumulator, layer or channel-specific requantization, and output clamping. A teaching reference may use 64-bit arithmetic to make accumulation semantics explicit. The following deliberately leaves the fixed-point multiplier in a separate named function so its contract can be tested independently.

```cpp
struct Requantization {
  std::int32_t multiplier;
  std::int32_t shift;
};

// Implement according to a selected, documented runtime's
// exact rounding, saturation, and shift convention.
std::int32_t requantize(std::int32_t value,
                        Requantization q);

Status dense_i8_reference(
    std::span<const std::int8_t> x,
    std::span<const std::int8_t> weights, // [n, m]
    std::span<const std::int32_t> bias,
    std::span<const Requantization> rq,
    std::span<std::int8_t> y,
    std::int32_t input_zero,
    std::int32_t output_zero) {
  const auto m = x.size(), n = y.size();
  if (m == 0 || n == 0 || n > weights.size() / m ||
      weights.size() != n * m || bias.size() != n ||
      rq.size() != n) return Status::shape_mismatch;
  for (std::size_t j = 0; j < n; ++j) {
    std::int64_t acc = bias[j];
    for (std::size_t i = 0; i < m; ++i)
      acc += (std::int32_t(x[i]) - input_zero) *
              std::int32_t(weights[j * m + i]);
    // Reject out-of-contract models rather than narrowing blindly.
    if (acc < INT32_MIN || acc > INT32_MAX)
      return Status::numerical_failure;
    std::int64_t r = requantize(std::int32_t(acc), rq[j]);
    r += output_zero;
    r = r < -128 ? -128 : (r > 127 ? 127 : r);
    y[j] = static_cast<std::int8_t>(r);
  }
  return Status::ok;
}
```

This example assumes symmetric weights (`weight_zero_point = 0`) and output activation spanning all signed INT8 codes; real model formats may require other conventions. This listing illustrates one operator interface but deliberately does not define `requantize`. **Chapter 12 and the companion `examples/int8_reference/` directory provide a separate, complete and host-tested reference implementation** with an explicit rational-multiplier convention. That convention is educational rather than TFLM/CMSIS-NN bit-exact; do not substitute the chapter 12 function in this skeleton without adapting the interface and quantization contract. `INT32_MIN`/`INT32_MAX` require the appropriate integer-limit header or an equivalent `std::numeric_limits`-based check.

## What a fixed-point requantizer must specify

A complete interface needs: multiplier representation; permitted shift range; behavior on negative values; rounding for ties; overflow policy for intermediate products; and saturation on extreme signed input. A function tested only on positive accumulators is under-tested. In production, use a documented implementation such as the one matched to CMSIS-NN when bit-exact compatibility with that runtime is needed.

To test the implementation, choose examples around every half-step boundary; generate randomized 32-bit accumulators with a seeded host oracle; and compare across multiple compilers or architecture targets. The model export step and runtime must share *identical* quantized arithmetic semantics.

## An MCU runtime need not parse a model format

An interpreter typically loads a serialized graph and selects operator implementations at runtime. A small fixed-purpose product can instead generate C/C++ arrays and a static operator schedule at build time. Both are valid architectures:

| Interpreter | Ahead-of-time (AOT) fixed graph |
|---|---|
| Supports multiple graph layouts | Graph must be rebuilt to change |
| More metadata and dispatch logic | Small dispatch overhead |
| Easier field model swaps if validated | Straightforward static memory ownership |
| Requires model compatibility layer | Code generator must be verified |

Do not assume AOT is always faster: large optimized matrix operators may dominate dispatch so thoroughly that the difference is negligible. Conversely, a tiny network with dozens of trivial operators may spend a measurable fraction of time in interpreter machinery. Measure on the target.

## Separate application and engine concerns

A useful architecture divides source into:

```text
platform/   clocks, DMA, timers, GPIO, monotonic time
capture/    sample acquisition, ring, timestamps, fault counters
dsp/        deterministic feature extraction
runtime/    tensors, graph, activation plan, kernel dispatch
kernels/    slow reference and optimized backends
model/      model bytes, metadata, test vectors
detect/     smoothing, debounce, freshness, actions
```

This boundary enables host tests of DSP and operators without including HAL or RTOS headers. The runtime should neither read an I²C sensor nor toggle an LED. It consumes a validated tensor and produces an output tensor plus status.

## The operator registry and versioning problem

A model graph may require Conv2D, DepthwiseConv2D, pooling, reshape, mean, softmax, and quantize/dequantize. An operator registry must identify not only the name but the supported input types, parameter fields, and versions. Unsupported variants should fail at initialization rather than produce undefined behavior at runtime. If a version bump changes quantization or padding semantics, the firmware's model acceptance test must catch it.

## Verification ladder

1. **Mathematical unit tests:** tiny tensors with hand-calculated outputs.
2. **Quantized parity:** compare every operator against a selected runtime's known answers.
3. **Graph parity:** compare intermediates and final outputs for fixed test vectors.
4. **Host sanitizers:** bounds, signed overflow, use-after-lifetime, uninitialized output.
5. **Target tests:** flash, RAM, cycles, numerical identity, buffer alignment.
6. **Application tests:** real sensor data and confidence policy under faults.

When introducing an optimized backend, run the same graph and test vectors through both kernels. An acceleration claim is meaningless if the implementations differ in accuracy, operator versions, or preprocessing.

**Exercises**

1. Add a checked multiplication helper for shape validation that rejects overflow of `size_t`.
2. Specify a `TensorView` using explicit strides and demonstrate a transposed view with no allocation.
3. Create three quantized dense test vectors: zero-point padding, negative bias, and saturated positive output.
