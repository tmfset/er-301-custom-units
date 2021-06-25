#include <od/config.h>
#include <hal/simd.h>
#include <util.h>

namespace compressor {
  struct Slew {
    inline float32x4_t process(
      uint32x4_t rising,
      float32x4_t rise,
      float32x4_t fall
    ) {
      auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      auto delta = vbslq_f32(rising, rise, vnegq_f32(fall));
      delta = sp * util::simd::invert(delta);

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
  };

  struct Compressor {
    template <int CC>
    inline float32x4_t excite(
      float32x4_t *signals
    ) {
      auto zero = vdupq_n_f32(0);
      auto output = vabdq_f32(signals[0], zero);
      for (int i = 1; i < CC; i++) {
        output = vmaxq_f32(
          output,
          vabdq_f32(signals[i], zero)
        );
      }
      return output;
    }

    inline void process(
      float32x4_t excite,
      float32x4_t threshold,
      float32x4_t ratio,
      float32x4_t rise,
      float32x4_t fall
    ) {
      auto zero = vdupq_n_f32(0);
      auto one = vdupq_n_f32(1);

      auto exciteDb = util::simd::toDecibels(excite);
      auto thresholdDb = util::simd::toDecibels(threshold);

      mActive = vcgtq_f32(exciteDb, thresholdDb);
      auto over = vmaxq_f32(exciteDb - thresholdDb, zero);
      mAmount = mSlew.process(mActive, rise, fall);
      auto ratioI = util::simd::invert(ratio);
      mReduction = util::simd::fromDecibels(over * mAmount * (ratioI - one));
    }

    inline float32x4_t compress(
      float32x4_t signal
    ) {
      return signal * mReduction;
    }

    uint32x4_t mActive;
    float32x4_t mAmount;
    float32x4_t mReduction;
    Slew mSlew;
  };
}