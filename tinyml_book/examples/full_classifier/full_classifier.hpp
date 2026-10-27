#pragma once
#include "int8_reference.hpp"
#include <array>
#include <cstdint>
namespace tinyml::example {
struct Result { Status status; std::array<std::int8_t,2> hidden; std::array<std::int8_t,2> logits; int label; };
Result infer(const std::array<std::int8_t,2>& features) noexcept;
}
