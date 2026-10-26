#pragma once
// Educational, C++20, heap-free quantized inference primitives.
// Fixed-point convention: round to nearest, ties away from zero.
// This is NOT a TFLM/CMSIS-NN bit-exact requantizer.
#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyml {
enum class Status : std::uint8_t {
  ok, invalid_shape, invalid_quantization, accumulator_overflow
};

struct Scale2 {
  // Positive rational multiplier numerator / 2^shift.
  // numerator in [1, INT32_MAX], shift in [0, 31].
  std::int32_t numerator;
  unsigned shift;
};

struct DenseParams {
  std::size_t inputs;
  std::size_t outputs;
  std::int32_t input_zero_point;
  std::int32_t output_zero_point;
  std::int32_t activation_min = -128;
  std::int32_t activation_max = 127;
};

// int64 intermediates; saturation to int32 only at the final step.
Status requantize(std::int32_t accumulator, Scale2 scale,
                  std::int32_t& result) noexcept;

// Matrix W is [outputs, inputs] row-major. Symmetric signed INT8 weights.
// Bias and scales are one element per output channel. Input/output may not
// alias weight/bias/scale memory. No allocation or I/O in either function.
Status dense(std::span<const std::int8_t> input,
             std::span<const std::int8_t> weights,
             std::span<const std::int32_t> biases,
             std::span<const Scale2> scales,
             std::span<std::int8_t> output,
             DenseParams p) noexcept;

struct Conv1DParams {
  std::size_t time_steps;
  std::size_t input_channels;
  std::size_t output_channels;
  std::size_t kernel_width;
  std::size_t stride;
  std::size_t pad_left;
  std::size_t pad_right;
  std::int32_t input_zero_point;
  std::int32_t output_zero_point;
  std::int32_t activation_min = -128;
  std::int32_t activation_max = 127;
};

// Layout: input [T,Cin], weights [Cout,K,Cin], output [Tout,Cout].
Status conv1d(std::span<const std::int8_t> input,
              std::span<const std::int8_t> weights,
              std::span<const std::int32_t> biases,
              std::span<const Scale2> scales,
              std::span<std::int8_t> output,
              Conv1DParams p) noexcept;
}
