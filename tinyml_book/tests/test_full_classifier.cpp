#include "full_classifier.hpp"
#include <array>
#include <cassert>
int main() {
  auto a=tinyml::example::infer(std::array<std::int8_t,2>{4,2});
  assert(a.status==tinyml::Status::ok && a.hidden[0]==3 && a.hidden[1]==1);
  assert(a.logits[0]==2 && a.logits[1]==-2 && a.label==0);
  auto b=tinyml::example::infer(std::array<std::int8_t,2>{-4,2});
  assert(b.status==tinyml::Status::ok && b.hidden[0]==0 && b.hidden[1]==0);
  assert(b.logits[0]==0 && b.logits[1]==0 && b.label==0);
}
