#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <util/math.h>

namespace dsp {
  #define DEFAULT_TIMER_DURATION 0.1f

  class Timer {
    public:
      inline Timer(float samplePeriod) :
          Timer(samplePeriod, DEFAULT_TIMER_DURATION) { }

      inline Timer(float samplePeriod, float seconds) :
          mSamplePeriod(samplePeriod) {
        setDuration(seconds);
      }

      inline void setDuration(float seconds) {
        mDelta = mSamplePeriod / util::fmax(seconds, mSamplePeriod);
      }

      inline uint32_t process(uint32_t reset) {
        mPhase -= mDelta;

        if (reset || mPhase <= 0) {
          mPhase = 1;
          return util::bcvt(true);
        }

        return util::bcvt(false);
      }

    private:
      const float mSamplePeriod;

      float mPhase = 0.0f;
      float mDelta = 0.0f;
  };

  namespace two {
    class Timer {
      public:
        inline Timer(float32x2_t samplePeriod) :
            Timer(samplePeriod, vdup_n_f32(DEFAULT_TIMER_DURATION)) { }

        inline Timer(float32x2_t samplePeriod, float32x2_t seconds) :
            mSamplePeriod(samplePeriod) {
          setDuration(seconds);
        }

        inline void setDuration(float32x2_t seconds) {
          mDelta = vmul_f32(mSamplePeriod, util::two::invert(vmax_f32(seconds, mSamplePeriod)));
        }

        inline uint32x2_t process(uint32x2_t reset) {
          mPhase = vsub_f32(mPhase, mDelta);

          auto doReset = vorr_u32(reset, vcle_f32(mPhase, vdup_n_f32(0)));
          mPhase = vbsl_f32(doReset, vdup_n_f32(1), mPhase);
          return doReset;
        }

      private:
        const float32x2_t mSamplePeriod;

        float32x2_t mPhase = vdup_n_f32(0);
        float32x2_t mDelta = vdup_n_f32(0);
    };
  }

  namespace four {
    class Timer {
      public:
        inline Timer(float32x4_t samplePeriod) :
            Timer(samplePeriod, vdupq_n_f32(DEFAULT_TIMER_DURATION)) { }

        inline Timer(float32x4_t samplePeriod, float32x4_t seconds) :
            mSamplePeriod(samplePeriod) {
          setDuration(seconds);
        }

        inline void setDuration(float32x4_t seconds) {
          mDelta = mSamplePeriod * util::four::invert(vmaxq_f32(seconds, mSamplePeriod));
        }

        inline uint32x4_t process(uint32x4_t reset) {
          mPhase -= mDelta;

          auto doReset = reset | vcleq_f32(mPhase, vdupq_n_f32(0));
          mPhase = vbslq_f32(doReset, vdupq_n_f32(1), mPhase);
          return doReset;
        }

      private:
        const float32x4_t mSamplePeriod;

        float32x4_t mPhase = vdupq_n_f32(0);
        float32x4_t mDelta = vdupq_n_f32(0);
    };
  }

  namespace simd {
    class Timer {
      public:
        inline Timer(float samplePeriod) :
            Timer(samplePeriod, DEFAULT_TIMER_DURATION) { }

        inline Timer(float samplePeriod, float seconds) :
            mSamplePeriod(samplePeriod) {
          setDuration(seconds);
        }

        inline void setDuration(float seconds) {
          mDelta = mSamplePeriod / util::fmax(seconds, mSamplePeriod);
        }

        inline uint32x4_t process(uint32x4_t reset) {
          uint32_t _reset[4], _out[4];
          vst1q_u32(_reset, reset);

          auto _phase = mPhase;
          for (int i = 0; i < 4; i++) {
            _phase -= mDelta;

            auto out = _reset[i] || _phase <= 0.0f;
            if (out) _phase = 1.0f;
            _out[i] = util::bcvt(out);
          }

          mPhase = _phase;
          return vld1q_u32(_out);
        }

      private:
        const float mSamplePeriod;

        float mPhase = 0;
        float mDelta = 0;
    };
  }
}