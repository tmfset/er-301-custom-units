#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <math.h>
#include "../util.h"

namespace slew {
  struct SlewRate {
    inline static SlewRate fromRiseFall(float rise, float fall, float sp) {
      rise = expf(-sp / util::fmax(rise, sp));
      fall = expf(-sp / util::fmax(fall, sp));
      return SlewRate { rise, fall };
    }

    inline static SlewRate fromRate(float rate, float sp) {
      rate = expf(-sp / util::fmax(rate, sp));
      return SlewRate { rate, rate };
    }

    inline SlewRate(float riseCoeff, float fallCoeff) :
      mRiseCoeff(riseCoeff),
      mFallCoeff(fallCoeff) { }

    inline float pick(bool rise) const {
      return rise ? mRiseCoeff : mFallCoeff;
    }

    const float mRiseCoeff;
    const float mFallCoeff;
  };

  struct Slew {
    inline float process(const SlewRate &rate, float target) {
      auto coeff = rate.pick(mValue < target);
      mValue = target + coeff * (mValue - target);
      return mValue;
    }

    inline void hardSet(float v) { mValue = v; }

    float mValue = 0;
  };

  namespace two {
    struct Slew {
      inline void setRiseFall(float rise, float fall) {
        auto sp = globalConfig.samplePeriod;
        rise = util::fmax(rise, sp);
        fall = util::fmax(fall, sp);

        auto coeff = vcombine_f32(vdup_n_f32(-sp / rise), vdup_n_f32(-sp / fall));
        coeff = util::simd::exp_f32(coeff);

        mRiseCoeff = vget_low_f32(coeff);
        mFallCoeff = vget_high_f32(coeff);
      }

      inline float32x2_t pick(uint32x2_t rise) const {
        return vbsl_f32(rise, mRiseCoeff, mFallCoeff);
      }

      inline float32x2_t process(float32x2_t target) {
        auto coeff = pick(vclt_f32(mValue, target));
        mValue = vadd_f32(target, vmul_f32(coeff, vsub_f32(mValue, target)));
        return mValue;
      }

      float32x2_t mValue = vdup_n_f32(0);
      float32x2_t mRiseCoeff = vdup_n_f32(0);
      float32x2_t mFallCoeff = vdup_n_f32(0);
    };
  }

  namespace four {
    struct Slew {
      inline void setRiseFall(float rise, float fall) {
        auto sp = globalConfig.samplePeriod;
        rise = util::fmax(rise, sp);
        fall = util::fmax(fall, sp);

        mRiseCoeff = util::simd::exp_f32(vdupq_n_f32(-sp / rise));
        mFallCoeff = util::simd::exp_f32(vdupq_n_f32(-sp / fall));
      }

      inline float32x4_t pick(uint32x4_t rise) const {
        return vbslq_f32(rise, mRiseCoeff, mFallCoeff);
      }

      inline float32x4_t process(float32x4_t target) {
        auto coeff = pick(vcltq_f32(mValue, target));
        mValue = target + coeff * (mValue - target);
        return mValue;
      }

      float32x4_t mValue = vdupq_n_f32(0);
      float32x4_t mRiseCoeff = vdupq_n_f32(0);
      float32x4_t mFallCoeff = vdupq_n_f32(0);
    };
  }
}