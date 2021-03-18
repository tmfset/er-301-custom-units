#pragma once
#include <hal/simd.h>

namespace lojik {
  inline int32_t mod(int32_t a, int32_t n) {
    return ((a % n) + n) % n;
  }

  inline int32_t clamp(int32_t value, int32_t min, int32_t max) {
    return (value > max) ? max : (value < min) ? min : value;
  }
}