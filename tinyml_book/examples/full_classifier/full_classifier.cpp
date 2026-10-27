#include "full_classifier.hpp"
namespace tinyml::example {
Result infer(const std::array<std::int8_t,2>& features) noexcept {
  constexpr std::array<std::int8_t,4> w1{1,1,1,-1};
  constexpr std::array<std::int8_t,4> w2{1,-1,-1,1};
  constexpr std::array<std::int32_t,2> bias{0,0};
  constexpr std::array<Scale2,2> q1{{{1,1},{1,1}}};
  constexpr std::array<Scale2,2> q2{{{1,0},{1,0}}};
  Result r{Status::ok,{},{},-1};
  r.status = dense(features,w1,bias,q1,r.hidden,{2,2,0,0,0,127});
  if (r.status != Status::ok) return r;
  r.status = dense(r.hidden,w2,bias,q2,r.logits,{2,2,0,0});
  if (r.status == Status::ok) r.label=(r.logits[0]>=r.logits[1]) ? 0:1;
  return r;
}
}
