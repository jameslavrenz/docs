#include "int8_reference.hpp"
#include <array>
#include <cstdio>
using namespace tinyml;
int main() {
  const std::array<std::int8_t,4> in = {1,2,3,4};
  const std::array<std::int8_t,12> w = {1,1,1,1, 2,0,-2,0, -1,1,-1,1};
  const std::array<std::int32_t,3> b = {0, 0, 2};
  const std::array<Scale2,3> q = {{{1,1},{1,1},{1,0}}};
  std::array<std::int8_t,3> out{};
  const Status status = dense(in,w,b,q,out,{4,3,0,0});
  if (status != Status::ok) return 1;
  std::printf("dense output = [%d, %d, %d]\n", int(out[0]),int(out[1]),int(out[2]));
  return 0;
}
