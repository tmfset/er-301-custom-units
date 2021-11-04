#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include "util.h"

namespace slew {
  struct Slew {
    inline void setRate(float rate, float sp) {
      setRiseFall(rate, rate, sp);
    }

    inline void setRiseFall(float rise, float fall, float sp) {
      rise = util::fmax(rise, sp);
      fall = util::fmax(fall, sp);

      mRiseCoeff = exp(-sp / rise);
      mFallCoeff = exp(-sp / fall);
    }

    inline float pick(bool rise) const {
      return rise ? mRiseCoeff : mFallCoeff;
    }

    inline float value() const {
      return mValue;
    }

    inline void process(float target) {
      auto coeff = pick(mValue < target);
      mValue = target + coeff * (mValue - target);
    }

    float mValue = 0;
    float mRiseCoeff = 0;
    float mFallCoeff = 0;
  };
}