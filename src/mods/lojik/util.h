#pragma once
#include <hal/simd.h>
#include <vector>

namespace lojik {
  inline float32x4_t invert(const float32x4_t x) {
    float32x4_t inv;
    // https://en.wikipedia.org/wiki/Division_algorithm#Newton.E2.80.93Raphson_division
    inv = vrecpeq_f32(x);
    // iterate 3 times for 24 bits of precision
    inv *= vrecpsq_f32(x, inv);
    inv *= vrecpsq_f32(x, inv);
    inv *= vrecpsq_f32(x, inv);
    return inv;
  }

  inline int mod(int a, int n) {
    return ((a % n) + n) % n;
  }

  inline int clamp(int value, int min, int max) {
    return (value > max) ? max : (value < min) ? min : value;
  }

  inline float fclamp(float value, float min, float max) {
    return (value > max) ? max : (value < min) ? min : value;
  }

  inline float32x4_t makeq_f32(float a, float b, float c, float d) {
    float32x4_t x = vdupq_n_f32(0);
    x = vsetq_lane_f32(a, x, 0);
    x = vsetq_lane_f32(b, x, 1);
    x = vsetq_lane_f32(c, x, 2);
    x = vsetq_lane_f32(d, x, 3);
    return x;
  }

  inline float32x4_t floor(const float32x4_t x) {
    return vcvtq_f32_s32(vcvtq_s32_f32(x));
  }

  struct Trigger {
    inline bool read(uint32_t high) {
      mTrigger = high & mEnable;
      mEnable = ~high;
      return mTrigger;
    }

    uint32_t mEnable  = 0;
    uint32_t mTrigger = 0;
  };
}