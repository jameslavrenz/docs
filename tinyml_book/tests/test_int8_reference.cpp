#include "int8_reference.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>
#include <vector>
using namespace tinyml;
#define REQUIRE(cond) do { if (!(cond)) {   std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   std::abort(); }} while (false)

void test_requant() {
  std::int32_t y{};
  REQUIRE(requantize( 3, {1,1}, y)==Status::ok && y== 2);
  REQUIRE(requantize(-3, {1,1}, y)==Status::ok && y==-2);
  REQUIRE(requantize(-1, {1,1}, y)==Status::ok && y==-1);
  REQUIRE(requantize( 1, {1,1}, y)==Status::ok && y== 1);
  REQUIRE(requantize(-5, {3,2}, y)==Status::ok && y==-4);
  REQUIRE(requantize(1,{0,1},y)==Status::invalid_quantization);
  REQUIRE(requantize(1,{1,32},y)==Status::invalid_quantization);
  REQUIRE(requantize(std::numeric_limits<std::int32_t>::max(), {std::numeric_limits<std::int32_t>::max(),0}, y)==Status::ok && y==std::numeric_limits<std::int32_t>::max());
  std::mt19937 rng(0xC0FFEE);
  for (int i=0;i<100000;i++) {
    const std::int32_t a=static_cast<std::int32_t>(rng());
    const std::int32_t n=static_cast<std::int32_t>((rng()&0x7fffffffU)|1U);
    const unsigned shift=1+(rng()%31);
    REQUIRE(requantize(a,{n,shift},y)==Status::ok);
    const long double ideal = static_cast<long double>(a)*n/(std::uint64_t{1}<<shift);
    const auto oracle = static_cast<std::int64_t>(ideal<0 ? -std::floor(-ideal+0.5L) : std::floor(ideal+0.5L));
    const auto clamped=std::max<std::int64_t>(std::numeric_limits<std::int32_t>::min(),std::min<std::int64_t>(std::numeric_limits<std::int32_t>::max(),oracle));
    REQUIRE(y==clamped);
  }
}
void test_dense() {
  const std::array<std::int8_t,4> x={1,2,3,4};
  const std::array<std::int8_t,12> w={1,1,1,1, 2,0,-2,0, -1,1,-1,1};
  const std::array<std::int32_t,3> b={0,0,2};
  const std::array<Scale2,3> q={{{1,1},{1,1},{1,0}}};
  std::array<std::int8_t,3> y{};
  REQUIRE(dense(x,w,b,q,y,{4,3,0,0})==Status::ok);
  REQUIRE((y==std::array<std::int8_t,3>{5,-2,4}));
  REQUIRE(dense(x,std::span<const std::int8_t>{},b,q,y,{4,3,0,0})==Status::invalid_shape);
  // Nonzero zero point: (10 - 10)*1 + (11 - 10)*2 = 2.
  const std::array<std::int8_t,2> z={10,11};
  const std::array<std::int8_t,2> wz={1,2};
  const std::array<std::int32_t,1> bz={0};
  const std::array<Scale2,1> qz={{{1,0}}};
  std::array<std::int8_t,1> yz{};
  REQUIRE(dense(z,wz,bz,qz,yz,{2,1,10,0})==Status::ok&&yz[0]==2);
  // 127*127*140000 exceeds INT32; detect before narrowing.
  const std::vector<std::int8_t> huge(140000,127);
  std::array<std::int8_t,1> h{};
  REQUIRE(dense(huge,huge,bz,qz,h,{140000,1,0,0})==Status::accumulator_overflow);
}
void test_conv1d() {
  const std::array<std::int8_t,4> x={1,2,3,4};
  const std::array<std::int8_t,3> kernel={1,2,1};
  const std::array<std::int32_t,1> bias={0};
  const std::array<Scale2,1> scale={{{1,0}}};
  std::array<std::int8_t,4> y{};
  const Conv1DParams p={4,1,1,3,1,1,1,0,0};
  REQUIRE(conv1d(x,kernel,bias,scale,y,p)==Status::ok);
  REQUIRE((y==std::array<std::int8_t,4>{4,8,12,11}));
  // With input zero point 5, padding must make zero contribution.
  const std::array<std::int8_t,4> shifted={6,7,8,9};
  REQUIRE(conv1d(shifted,kernel,bias,scale,y,{4,1,1,3,1,1,1,5,0})==Status::ok);
  REQUIRE((y==std::array<std::int8_t,4>{4,8,12,11}));
  REQUIRE(conv1d(x,kernel,bias,scale,y,{4,1,1,3,0,1,1,0,0})==Status::invalid_shape);
}
int main() {test_requant();test_dense();test_conv1d();}
