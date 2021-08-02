#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>

namespace env {
  namespace four {
    inline float32x4_t slew_coeff(const float32x4_t time) {
      const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      const auto timeI = util::simd::invert(vmaxq_f32(time, sp));
      return util::simd::exp_f32(-sp * timeI);
    }

    struct Coefficients {
      inline void track(
        const uint32x4_t gate,
        const Coefficients &other
      ) {
        mRiseCoeff.track(gate, other.mRiseCoeff);
        mFallCoeff.track(gate, other.mFallCoeff);
      }

      inline float32x4_t pick(const uint32x4_t rise) {
        return vbslq_f32(rise, mRiseCoeff.value(), mFallCoeff.value());
      }

      inline void configure(
        const float32x4_t rise,
        const float32x4_t fall
      ) {
        mRiseCoeff.set(slew_coeff(rise));
        mFallCoeff.set(slew_coeff(fall));
      }

      util::four::TrackAndHold mRiseCoeff { 0 };
      util::four::TrackAndHold mFallCoeff { 0 };
    };

    struct SlewEnvelope {
      inline float32x4_t process(
        const uint32x4_t gate,
        const Coefficients& cf
      ) {
        auto zero = vdupq_n_f32(0.0f);
        auto one  = vdupq_n_f32(1.0f);

        auto reset  = vcgtq_f32(mValue, vdupq_n_f32(0.999));
        auto high   = mLatch.read(gate, reset);

        auto coeff  = cf.pick(high);
        auto target = vbslq_f32(high, one, zero);

        // mVAlue = target + mCf.pick(high) * (mValue - target);
        mValue = coeff * mValue + (one - coeff) * target;
        return mValue;
      }

      util::FourLatch mLatch;
      float32x4_t mValue = vdupq_n_f32(0);
    };
  }

  struct SlewEnvelope {
    inline void setRiseFall(float rise, float fall) {
      auto sp = globalConfig.samplePeriod;
      rise = util::fmax(rise, sp);
      fall = util::fmax(fall, sp);

      mRiseCoeff = exp(-sp / rise);
      mFallCoeff = exp(-sp / fall);
    }

    inline float32x4_t process(uint32x4_t gate) {
      float _last = mValue,
            rc    = mRiseCoeff,
            fc    = mFallCoeff;

      uint32_t _gate[4];
      vst1q_u32(_gate, gate);

      float _value[4];

      for (int i = 0; i < 4; i++) {
        auto reset = _last >= 0.999f ? 0xffffffff : 0;
        auto high = mLatch.read(_gate[i], reset);

        float c, target;
        if (high) { c = rc; target = 1.0f; }
        else { c = fc; target = 0.0f; }

        _last = c * _last + (1.0f - c) * target;

        _value[i] = _last;
      }

      mValue = _last;
      return vld1q_f32(_value);
    }

    util::Latch mLatch;
    float mValue = 0;
    float mRiseCoeff = 0;
    float mFallCoeff = 0;
  };
}