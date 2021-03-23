#pragma once
#include <hal/simd.h>

namespace lojik {
  inline int mod(int a, int n) {
    return ((a % n) + n) % n;
  }

  inline int clamp(int value, int min, int max) {
    return (value > max) ? max : (value < min) ? min : value;
  }

  inline float fclamp(float value, float min, float max) {
    return (value > max) ? max : (value < min) ? min : value;
  }
}