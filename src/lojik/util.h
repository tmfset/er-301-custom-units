#pragma once
#include <hal/simd.h>
#include <vector>

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

  struct Trigger {
    inline bool readTrigger(bool high) {
      if (high) { mTrigger = mEnable; mEnable = false; }
      else      { mTrigger = false;   mEnable = true; }
      return mTrigger;
    }

    inline void readTriggers(bool *out, const uint32x4_t high) {
      uint32_t _h[4];
      vst1q_u32(_h, high);
      for (int i = 0; i < 4; i++) {
        out[i] = readTrigger(_h[i]);
      }
    }

    inline void readTriggersU(uint32_t *out, const uint32x4_t high) {
      uint32_t _h[4];
      vst1q_u32(_h, high);
      for (int i = 0; i < 4; i++) {
        out[i] = readTrigger(_h[i]);
      }
    }

    bool mEnable  = false;
    bool mTrigger = false;
  };
}