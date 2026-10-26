#include "int8_reference.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace tinyml {
namespace {
constexpr auto kI32Min = std::numeric_limits<std::int32_t>::min();
constexpr auto kI32Max = std::numeric_limits<std::int32_t>::max();

bool product_size(std::size_t a, std::size_t b,
                  std::size_t& result) noexcept {
  if (a && b > std::numeric_limits<std::size_t>::max() / a) return false;
  result = a * b;
  return true;
}

bool add_size(std::size_t a, std::size_t b,
              std::size_t& result) noexcept {
  if (b > std::numeric_limits<std::size_t>::max() - a) return false;
  result = a + b;
  return true;
}

std::int8_t clipped(std::int32_t x, std::int32_t lo,
                    std::int32_t hi) noexcept {
  return static_cast<std::int8_t>(std::clamp(x, lo, hi));
}

bool valid_activation(std::int32_t lo, std::int32_t hi) noexcept {
  return lo >= -128 && hi <= 127 && lo <= hi;
}

bool in_i32(std::int64_t value) noexcept {
  return value >= kI32Min && value <= kI32Max;
}

bool in_i8(std::int32_t z) noexcept {
  return z >= -128 && z <= 127;
}

} // namespace

Status requantize(std::int32_t accumulator, Scale2 scale,
                  std::int32_t& result) noexcept {
  if (scale.numerator <= 0 || scale.shift > 31)
    return Status::invalid_quantization;
  const std::int64_t p = std::int64_t(accumulator) * scale.numerator;
  // p fits int64: |INT32_MIN * INT32_MAX| < 2^62.
  const std::int64_t magnitude = p < 0 ? -p : p;
  const std::int64_t divisor = std::int64_t{1} << scale.shift;
  const std::int64_t rounded = (magnitude + divisor / 2) / divisor;
  const std::int64_t signed_value = p < 0 ? -rounded : rounded;
  // The shifted intermediate may be outside int32. Saturate explicitly.
  result = static_cast<std::int32_t>(
      std::clamp(signed_value, std::int64_t(kI32Min), std::int64_t(kI32Max)));
  return Status::ok;
}

Status dense(std::span<const std::int8_t> input,
             std::span<const std::int8_t> weights,
             std::span<const std::int32_t> biases,
             std::span<const Scale2> scales,
             std::span<std::int8_t> output, DenseParams p) noexcept {
  std::size_t nweights = 0;
  if (!p.inputs || !p.outputs || !product_size(p.inputs, p.outputs, nweights) ||
      input.size() != p.inputs || output.size() != p.outputs ||
      weights.size() != nweights || biases.size() != p.outputs ||
      scales.size() != p.outputs || !in_i8(p.input_zero_point) ||
      !in_i8(p.output_zero_point) ||
      !valid_activation(p.activation_min, p.activation_max))
    return Status::invalid_shape;
  for (std::size_t o = 0; o < p.outputs; ++o) {
    std::int64_t acc = biases[o];
    for (std::size_t i = 0; i < p.inputs; ++i)
      acc += (std::int64_t(input[i]) - p.input_zero_point) *
             std::int64_t(weights[o * p.inputs + i]);
    if (!in_i32(acc)) return Status::accumulator_overflow;
    std::int32_t scaled = 0;
    const Status st = requantize(static_cast<std::int32_t>(acc), scales[o], scaled);
    if (st != Status::ok) return st;
    const std::int64_t with_zp = std::int64_t(scaled) + p.output_zero_point;
    const auto bounded = static_cast<std::int32_t>(
      std::clamp(with_zp, std::int64_t(p.activation_min),
                 std::int64_t(p.activation_max)));
    output[o] = clipped(bounded, p.activation_min, p.activation_max);
  }
  return Status::ok;
}

Status conv1d(std::span<const std::int8_t> input,
              std::span<const std::int8_t> weights,
              std::span<const std::int32_t> biases,
              std::span<const Scale2> scales,
              std::span<std::int8_t> output, Conv1DParams p) noexcept {
  std::size_t in_count{}, kc{}, weight_count{}, padded{}, out_count{};
  if (!p.time_steps || !p.input_channels || !p.output_channels ||
      !p.kernel_width || !p.stride ||
      !product_size(p.time_steps, p.input_channels, in_count) ||
      !product_size(p.kernel_width, p.input_channels, kc) ||
      !product_size(kc, p.output_channels, weight_count) ||
      !add_size(p.time_steps, p.pad_left, padded) ||
      !add_size(padded, p.pad_right, padded) || padded < p.kernel_width ||
      input.size() != in_count || weights.size() != weight_count ||
      !in_i8(p.input_zero_point) || !in_i8(p.output_zero_point) ||
      !valid_activation(p.activation_min, p.activation_max))
    return Status::invalid_shape;
  const std::size_t out_time = 1 + (padded - p.kernel_width) / p.stride;
  if (!product_size(out_time, p.output_channels, out_count) ||
      output.size() != out_count || biases.size() != p.output_channels ||
      scales.size() != p.output_channels)
    return Status::invalid_shape;
  // Each output uses signed 64-bit accumulation; reject narrowing overflow.
  for (std::size_t t = 0; t < out_time; ++t) {
    for (std::size_t o = 0; o < p.output_channels; ++o) {
      std::int64_t acc = biases[o];
      for (std::size_t k = 0; k < p.kernel_width; ++k) {
        const std::size_t padded_t = t * p.stride + k;
        if (padded_t < p.pad_left) continue;
        const std::size_t t0 = padded_t - p.pad_left;
        if (t0 >= p.time_steps) continue; // quantized padding => real zero
        for (std::size_t c = 0; c < p.input_channels; ++c) {
          const auto x = std::int32_t(input[t0*p.input_channels+c]) -
                         p.input_zero_point;
          const auto w = weights[(o*p.kernel_width+k)*p.input_channels+c];
          acc += std::int64_t(x) * std::int64_t(w);
        }
      }
      if (!in_i32(acc)) return Status::accumulator_overflow;
      std::int32_t scaled{};
      const Status st = requantize(static_cast<std::int32_t>(acc), scales[o], scaled);
      if (st != Status::ok) return st;
      const auto v = std::clamp(std::int64_t(scaled) + p.output_zero_point,
                                std::int64_t(p.activation_min),
                                std::int64_t(p.activation_max));
      output[t*p.output_channels+o] = static_cast<std::int8_t>(v);
    }
  }
  return Status::ok;
}
}
