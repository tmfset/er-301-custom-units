#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <util.h>

namespace slew {
  struct Slew {
    inline void setRiseFall(float rise, float fall) {
      auto sp = globalConfig.samplePeriod;
      rise = util::fmax(rise, sp);
      fall = util::fmax(fall, sp);

      mRiseCoeff = exp(-sp / rise);
      mFallCoeff = exp(-sp / fall);
    }

    inline float32x4_t process(float32x4_t to) {
      float _last = mValue,
            rc    = mRiseCoeff,
            fc    = mFallCoeff;

      float _target[4];
      vst1q_f32(_target, to);

      for (int i = 0; i < 4; i++) {
        auto target = _target[i];

        auto c = target > _last ? rc : fc;
        _last = c * _last + (1.0f - c) * target;

        _target[i] = _last;
      }

      mValue = _last;
      return vld1q_f32(_target);
    }

    float mValue = 0;
    float mRiseCoeff = 0;
    float mFallCoeff = 0;
  };
}