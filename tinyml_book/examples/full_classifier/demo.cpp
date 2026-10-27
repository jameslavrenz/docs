#include "full_classifier.hpp"
#include <array>
#include <cstdio>
int main() {
  constexpr std::array<std::int8_t,2> features{4,2};
  auto r=tinyml::example::infer(features);
  if (r.status != tinyml::Status::ok) return 1;
  std::printf("hidden=[%d,%d] logits=[%d,%d] class=%d\n",int(r.hidden[0]),int(r.hidden[1]),int(r.logits[0]),int(r.logits[1]),r.label);
  return 0;
}
