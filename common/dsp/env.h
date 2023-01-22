#pragma once

#include <od/config.h>
#include <util/math.h>
#include <dsp/latch.h>

namespace env {
  namespace four {
    inline float32x4_t slew_coeff(const float32x4_t time) {
      const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      const auto timeI = util::four::invert(vmaxq_f32(time, vdupq_n_f32(0.0005)));
      return util::simd::exp_f32(-sp * timeI);
    }

    inline float32x4_t rate_coeff(const float32x4_t time) {
      const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      const auto timeI = util::four::invert(vmaxq_f32(time, sp));
      return sp * timeI;
    }

    struct Coefficients {
      inline void track(
        const uint32x4_t gate,
        const Coefficients &other
      ) {
        mRiseCoeff.track(gate, other.mRiseCoeff);
        mFallCoeff.track(gate, other.mFallCoeff);
      }

      inline float32x4_t pick(const uint32x4_t rise) const {
        return vbslq_f32(rise, mRiseCoeff.value(), mFallCoeff.value());
      }

      inline void configure(
        const float32x4_t rise,
        const float32x4_t fall
      ) {
        mRiseCoeff.set(slew_coeff(rise));
        mFallCoeff.set(slew_coeff(fall));
      }

      inline void configureRate(
        const float32x4_t rise,
        const float32x4_t fall
      ) {
        mRiseCoeff.set(rate_coeff(rise));
        mFallCoeff.set(rate_coeff(fall));
      }

      dsp::four::TrackAndHold mRiseCoeff { 0 };
      dsp::four::TrackAndHold mFallCoeff { 0 };
    };

    struct SlewEnvelope {
      inline float32x4_t process(
        const uint32x4_t gate,
        const Coefficients& cf
      ) {
        //auto high = gate;
        auto reset  = vcgtq_f32(mValue, vdupq_n_f32(0.632));
        auto high   = mLatch.process(gate, reset);

        auto coeff  = cf.pick(high);
        auto target = vcvtq_n_f32_u32(high, 32);

        // v = vc + t(1 - c)
        // v = vc + t - tc
        // v = t - tc + vc
        // v = t - c(t - v)
        mValue = target - coeff * (target - mValue);
        //mValue = target + coeff * (mValue - target);
        
        return mValue;
      }

      dsp::four::SRLatch mLatch;
      float32x4_t mValue = vdupq_n_f32(0);
    };

    struct EnvFollower {
      inline float32x4_t process(
        const float32x4_t input,
        const Coefficients& cf
      ) {
        auto excite = vabsq_f32(input);
        auto high = vcgtq_f32(excite, mValue);
        auto coeff = cf.pick(high);
        mValue = excite + coeff * (mValue - excite);
        //mValue = coeff * mValue + (one - coeff) * excite;
        return mValue;
      }

      float32x4_t mValue = vdupq_n_f32(0);
    };

    struct AttackDecay {
      inline float32x4_t process(
        const uint32x4_t gate,
        const Coefficients& cf
      ) {
        auto target = vcvtq_n_f32_u32(mLatch.process(gate, vmvnq_u32(mRising)), 32);
        auto rising   = vcgtq_f32(target, mValue);
        mRising = rising;

        auto distance = vabdq_f32(target, mValue);
        auto limit    = cf.pick(rising);

        auto delta   = vminq_f32(distance, limit);
        mValue += vbslq_f32(rising, delta, -delta);;
        return mValue * mValue * mValue;
      }

      dsp::four::SRLatch mLatch;
      uint32x4_t mRising = vdupq_n_u32(0);
      float32x4_t mValue = vdupq_n_f32(0);
    };
  }

  namespace two {
    inline float32x2_t slew_coeff(const float32x2_t time) {
      const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      const auto timeI = util::four::invert(vmaxq_f32(vcombine_f32(time, time), sp));
      return vget_low_f32(util::simd::exp_f32(-sp * timeI));
    }

    struct Coefficients {
      inline float32x2_t pick(const uint32x2_t rise) const {
        return vbsl_f32(rise, mRiseCoeff, mFallCoeff);
      }

      inline void configure(
        const float32x2_t rise,
        const float32x2_t fall
      ) {
        mRiseCoeff = slew_coeff(rise);
        mFallCoeff = slew_coeff(fall);
      }

      float32x2_t mRiseCoeff;
      float32x2_t mFallCoeff;
    };

    struct EnvFollower {
      inline float32x2_t process(
        const float32x2_t input,
        const Coefficients& cf
      ) {
        auto excite = vabs_f32(input);
        auto high = vcgt_f32(excite, mValue);
        auto coeff = cf.pick(high);
        mValue = vmla_f32(excite, coeff, vsub_f32(mValue, excite));
        //mValue = coeff * mValue + (one - coeff) * excite;
        return mValue;
      }

      float32x2_t mValue = vdup_n_f32(0);
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

    dsp::SRLatch mLatch;
    float mValue = 0;
    float mRiseCoeff = 0;
    float mFallCoeff = 0;
  };
}