#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <util.h>
#include <filter.h>

namespace compressor {
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

  // For reference,
  // https://www.eecs.qmul.ac.uk/~josh/documents/2012/GiannoulisMassbergReiss-dynamicrangecompression-JAES2012.pdf
  struct Compressor {

    inline void setRiseFall(float rise, float fall) {
      mSlew.setRiseFall(rise, rise + fall);
    }

    inline void setThresholdRatio(float threshold, float ratio, bool makeupEnabled) {
      auto thresholdDb = util::toDecibels(threshold);
      auto ratioI      = 1 / util::fmax(ratio, 1);
      auto over        = -thresholdDb;
      auto makeupDb    = util::fmax(over - over * ratioI, 0);
      auto makeup      = util::fromDecibels(makeupDb);

      mMakeup      = makeupEnabled ? makeup : 1.0f;
      mMakeupDb    = makeupDb;
      mThresholdDb = thresholdDb;
      mRatioI      = ratioI;
    }

    inline void excite(float32x4_t excite) {
      auto exciteDb    = util::simd::toDecibels(excite);
      auto thresholdDb = vdupq_n_f32(mThresholdDb);
      auto ratioI      = vdupq_n_f32(mRatioI);
      auto over        = vmaxq_f32(exciteDb - thresholdDb, vdupq_n_f32(0));
      auto slew        = mSlew.process(over - over * ratioI);

      mActive    = vcgtq_f32(slew, vdupq_n_f32(0.001));
      mReduction = util::simd::fromDecibels(vnegq_f32(slew));
    }

    inline float32x4_t makeup(float32x4_t signal) {
      return signal * mMakeup;
    }

    inline float32x4_t compress(float32x4_t signal) {
      return signal * mReduction;
    }

    uint32x4_t  mActive;
    float32x4_t mReduction;

    float mThresholdDb;
    float mRatioI;
    float mMakeup;
    float mMakeupDb;

    Slew mSlew;
  };
}