#include <od/config.h>
#include <hal/simd.h>
#include <util.h>

namespace compressor {
  struct Slew {
    inline void setRiseFall(float32x4_t rise, float32x4_t fall) {
      auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      mRiseDelta = sp * util::simd::invert(rise);
      mFallDelta = vnegq_f32(sp * util::simd::invert(fall));
    }

    inline float32x4_t process(uint32x4_t rising) {
      auto delta = vbslq_f32(rising, mRiseDelta, mFallDelta);
      float _out[4], _o = mValue;
      vst1q_f32(_out, delta);
      for (int i = 0; i < 4; i++) {
        _o += _out[i];
        if (_o >= 1) _o = 1;
        if (_o <= 0) _o = 0;
        _out[i] = _o;
      }

      mValue = _o;
      return vld1q_f32(_out);
    }

    float mValue = 0;
    float32x4_t mRiseDelta;
    float32x4_t mFallDelta;
  };

  struct Compressor {

    inline void setRiseFall(float32x4_t rise, float32x4_t fall) {
      mSlew.setRiseFall(rise, fall);
    }

    inline void setThresholdRatio(float32x4_t threshold, float32x4_t ratio, uint32x4_t enableMakeupGain) {
      mThresholdDb = util::simd::toDecibels(threshold);
      mRatioIMinusOne = util::simd::invert(ratio) - vdupq_n_f32(1);

      auto over = vmaxq_f32(vdupq_n_f32(0) - mThresholdDb, vdupq_n_f32(0));
      mMakeupGain = util::simd::invert(util::simd::fromDecibels(over * mRatioIMinusOne));
      mMakeupGain = vbslq_f32(enableMakeupGain, mMakeupGain, vdupq_n_f32(1));
    }

    inline void excite(float32x4_t excite) {
      mLoudness = util::simd::toDecibels(excite);

      mActive = vcgtq_f32(mLoudness, mThresholdDb);
      mSlewAmount = mSlew.process(mActive);

      auto over = vmaxq_f32(mLoudness - mThresholdDb, vdupq_n_f32(0));
      mReductionAmount = util::simd::fromDecibels(over * mSlewAmount * mRatioIMinusOne);
    }

    inline float32x4_t compress(float32x4_t signal) {
      return signal * mReductionAmount;
    }

    uint32x4_t  mActive;
    float32x4_t mThresholdDb;
    float32x4_t mRatioIMinusOne;
    float32x4_t mMakeupGain;
    float32x4_t mSlewAmount;
    float32x4_t mLoudness;
    float32x4_t mReductionAmount;

    Slew mSlew;
  };
}