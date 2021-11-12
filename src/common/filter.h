#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <util.h>
#include <od/AudioThread.h>

namespace filter {
  enum Processing {
    FRAME = 1,
    AUDIO = 2
  };

  // For reference, https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/
  namespace onepole {
    namespace four {
      struct Lowpass {
        const float32x4_t npi2sp = vdupq_n_f32(-2.0f * M_PI * globalConfig.samplePeriod);

        inline Lowpass() {}
        inline Lowpass(float f0) { setFrequency(vdupq_n_f32(f0)); }

        inline void setFrequency(float32x4_t f0) {
          mB1 = util::simd::exp_f32(f0 * npi2sp);
          mA0 = vdupq_n_f32(1.0f) - mB1;
        }

        inline float32x4_t process(const float32x4_t input) {
          mZ1 = input * mA0 + mZ1 * mB1;
          return mZ1;
        }

        float32x4_t mZ1 = vdupq_n_f32(0);
        float32x4_t mB1 = vdupq_n_f32(0);
        float32x4_t mA0 = vdupq_n_f32(1);
      };
    }

    namespace two {
      struct Lowpass {
        const float32x2_t npi2sp = vdup_n_f32(-2.0f * M_PI * globalConfig.samplePeriod);

        inline Lowpass() {}
        inline Lowpass(float f0) { setFrequency(vdup_n_f32(f0)); }

        inline void setFrequency(float32x2_t f0) {
          mB1 = util::two::exp_f32(vmul_f32(f0, npi2sp));
          mA0 = vsub_f32(vdup_n_f32(1.0f), mB1);
        }

        inline float32x2_t process(const float32x2_t input) {
          mZ1 = vadd_f32(vmul_f32(input, mA0), vmul_f32(mZ1, mB1));
          return mZ1;
        }

        float32x2_t mZ1 = vdup_n_f32(0);
        float32x2_t mB1 = vdup_n_f32(0);
        float32x2_t mA0 = vdup_n_f32(1);
      };

      struct DCBlocker {
        inline float32x2_t process(const float32x2_t input) {
          return vsub_f32(input, mFilter.process(input));
        }

        Lowpass mFilter { 1.0f / (float)globalConfig.sampleRate };
      };
    }

    struct Filter {
      const float32x4_t npi2sp = vdupq_n_f32(-2.0f * M_PI * globalConfig.samplePeriod);

      inline void setFrequency(float32x4_t f0) {
        mCoeff = util::simd::exp_f32(f0 * npi2sp);
      }

      inline float32x4_t process(const float32x4_t input) {
        float x[4], cf[4];
        vst1q_f32(x, input);
        vst1q_f32(cf, mCoeff);
        for (int i = 0; i < 4; i++) {
          auto c = cf[i];
          mZ1 = x[i] * (1.0f - c) + mZ1 * c;
          x[i] = mZ1;
        }
        return vld1q_f32(x);
      }

      float mZ1 = 0.0f;
      float32x4_t mCoeff;
    };
  }

  // Reference:
  //   https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
  //   https://github.com/FredAntonCorvest/Common-DSP
  namespace svf {
    #define INV_ROOT_TWO 0.70710678118f
    #define GLOBAL_MIN_Q logf(INV_ROOT_TWO)
    #define GLOBAL_MAX_Q logf(1000.0f)
    #define PI_SP M_PI * globalConfig.samplePeriod

    namespace four {
      struct Coefficients {
        inline Coefficients() {}

        inline Coefficients(float32x4_t f0, float32x4_t vpo, float32x4_t q) {
          configure(f0, vpo, q);
        }

        inline void configure(float32x4_t f0, float32x4_t vpo, float32x4_t q) {
          q = util::four::lerpi(vdupq_n_f32(GLOBAL_MIN_Q), vdupq_n_f32(GLOBAL_MAX_Q), q);
          q = util::simd::fast_exp_f32(q);
          f0 = util::four::vpo_scale_limited(f0, vpo);
          configure(f0, q);
        }

        inline void configure(float32x4_t f0, float32x4_t q) {
          auto k = util::four::invert(q);

          auto g = util::four::tan(f0 * vdupq_n_f32(PI_SP));
          auto a1 = util::four::invert(vdupq_n_f32(1.0f) + g * (g + k));
          auto a2 = g * a1;
          auto a3 = g * a2;

          mKA123 = { k, a1, a2, a3 };
        }

        inline float32x4_t k()  const { return mKA123.val[0]; }
        inline float32x4_t a1() const { return mKA123.val[1]; }
        inline float32x4_t a2() const { return mKA123.val[2]; }
        inline float32x4_t a3() const { return mKA123.val[3]; }

        float32x4x4_t mKA123;
      };

      struct Lowpass {
        inline void configure(float32x4_t f0, float32x4_t q) {
          mCf.configure(f0, q);
        }

        inline float32x4_t process(
          const float32x4_t x
        ) {
          auto v3 = x - mIc2;
          auto v1 = mCf.a1() * mIc1 + mCf.a2() * v3;
          auto v2 = mIc2 + mCf.a2() * mIc1 + mCf.a3() * v3;

          auto two = vdupq_n_f32(2);
          mIc1 = two * v1 - mIc1;
          mIc2 = two * v2 - mIc2;

          return v2;
        }

        Coefficients mCf;
        float32x4_t mIc1 = vdupq_n_f32(0);
        float32x4_t mIc2 = vdupq_n_f32(0);
      };

      struct Filter {
        inline void process(
          const Coefficients &cf,
          const float32x4_t x
        ) {
          auto v3 = x - mIc2;
          auto v1 = cf.a1() * mIc1 + cf.a2() * v3;
          auto v2 = mIc2 + cf.a2() * mIc1 + cf.a3() * v3;

          auto two = vdupq_n_f32(2);
          mIc1 = two * v1 - mIc1;
          mIc2 = two * v2 - mIc1;

          mLp = v2;
          mBp = v1;
          mHp = x - cf.k() * mBp - mLp;
        }

        float32x4_t mLp;
        float32x4_t mBp;
        float32x4_t mHp;

        float32x4_t mIc1 = vdupq_n_f32(0);
        float32x4_t mIc2 = vdupq_n_f32(0);
      };
    }

    namespace two {
      struct Coefficients {
        inline Coefficients(float32x2x4_t ka123) :
          mKA123(ka123) { }

        inline float32x2_t k()  const { return mKA123.val[0]; }
        inline float32x2_t a1() const { return mKA123.val[1]; }
        inline float32x2_t a2() const { return mKA123.val[2]; }
        inline float32x2_t a3() const { return mKA123.val[3]; }

        const float32x2x4_t mKA123;
      };

      struct Filter {
        inline float32x2_t process(const Coefficients &cf, const util::two::ThreeWay &tw, float32x2_t x) {
          auto ic1 = mIc1;
          auto ic2 = mIc2;

          auto v3 = vsub_f32(x, ic2);
          auto v1 = vadd_f32(vmul_f32(cf.a1(), ic1), vmul_f32(cf.a2(), v3));
          auto v2 = vadd_f32(ic2, vadd_f32(vmul_f32(cf.a2(), ic1), vmul_f32(cf.a3(), v3)));

          mIc1 = vsub_f32(util::two::twice(v1), ic1);
          mIc2 = vsub_f32(util::two::twice(v2), ic2);

          auto lp = v2;
          auto bp = v1;
          auto hp = vsub_f32(vsub_f32(x, vmul_f32(cf.k(), bp)), lp);

          return tw.mix(lp, bp, hp);
        }

        float32x2_t mIc1 = vdup_n_f32(0);
        float32x2_t mIc2 = vdup_n_f32(0);
      };
    }
  }

  namespace biquad {
    enum Type {
      LOWPASS  = 1,
      BANDPASS = 2,
      HIGHPASS = 3
    };

    namespace four {
      struct Coefficients {
        inline void configure(const float32x4_t f0) {
          auto theta = vmaxq_f32(f0, vdupq_n_f32(1)) * vdupq_n_f32(2.0f * M_PI * globalConfig.samplePeriod);
          float32x4_t sinTheta, cosTheta;
          util::simd::sincos_f32(theta, &sinTheta, &cosTheta);

          auto k = vdupq_n_f32(0.70710678118f);

          auto half = vdupq_n_f32(0.5);
          auto one = vdupq_n_f32(1);

          mB1 = one - cosTheta;
          mB0 = mB1 * half;
          mB2 = mB0;

          auto a = sinTheta * k;
          mA0I = util::four::invert(one + a);
          mA1 = vdupq_n_f32(-2) * cosTheta;
          mA2 = one - a;
        }

        float32x4_t mA0I;
        float32x4_t mA1;
        float32x4_t mA2;

        float32x4_t mB0;
        float32x4_t mB1;
        float32x4_t mB2;
      };

      struct Lowpass {
        inline void configure(const float32x4_t f0) {
          mCf.configure(f0);
        }

        inline float32x4_t process(const float32x4_t signal) {
          auto out = mCf.mB0 * signal;
          out     += mCf.mB1 * mXz1;
          out     += mCf.mB2 * mXz2;
          out     -= mCf.mA1 * mYz1;
          out     -= mCf.mA2 * mYz2;

          mXz2 = mXz1;
          mYz2 = mYz1;

          mXz1 = signal;
          mYz1 = out;

          return out;
        }

        Coefficients mCf;
        float32x4_t mXz1 = vdupq_n_f32(0);
        float32x4_t mXz2 = vdupq_n_f32(0);
        float32x4_t mYz1 = vdupq_n_f32(0);
        float32x4_t mYz2 = vdupq_n_f32(0);
      };
    }

    // https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
    // https://arachnoid.com/BiQuadDesigner/index.html
    class Coefficients {
      public:
        inline void update(
          const float32x4_t vpo,
          const float32x4_t f0,
          const float32x4_t _q,
          const Type t
        ) {
          auto f = util::four::vpo_scale_limited(f0, vpo);
          auto q = util::four::exp_ns_f32(_q, 0.70710678118f, 500.0f);

          auto theta = f * vdupq_n_f32(2.0f * M_PI * globalConfig.samplePeriod);
          float32x4_t sinTheta, cosTheta;
          util::simd::sincos_f32(theta, &sinTheta, &cosTheta);

          auto qI = util::four::invert(q);

          auto one  = vdupq_n_f32(1.0f);
          auto sh = sinTheta * vdupq_n_f32(0.5f);
          auto alpha = sh * qI;
          auto a0 = one + alpha;
          auto a1 = vnegq_f32(vdupq_n_f32(2.0f) * cosTheta);
          auto a2 = one - alpha;
          writeA(a1, a2);

          auto a0I = util::four::invert(a0);
          vst1q_f32(mAI, a0I);

          float32x4_t c, ch;
          switch (t) {
            case BANDPASS:
              writeB(sh, vdupq_n_f32(0), vnegq_f32(sh));
              break;

            case HIGHPASS:
              c  = vdupq_n_f32(1.0f) + cosTheta;
              ch = c * vdupq_n_f32(0.5f);
              writeB(ch, vnegq_f32(c), ch);
              break;

            default: // LOWPASS
              c  = vdupq_n_f32(1.0f) - cosTheta;
              ch = c * vdupq_n_f32(0.5f);
              writeB(ch, c, ch);
              break;
          }
        }

        inline float32x4x4_t a() const { return vld4q_f32(mA); }
        inline float32x4x4_t b() const { return vld4q_f32(mB); }
        inline const float *aI() const { return mAI; }

      private:
        inline void writeA(
          const float32x4_t a1,
          const float32x4_t a2
        ) {
          vst1q_f32(mA + 0,  vdupq_n_f32(0.0f));
          vst1q_f32(mA + 4,  vdupq_n_f32(0.0f));
          vst1q_f32(mA + 8,  a2);
          vst1q_f32(mA + 12, a1);
        }

        inline void writeB(
          const float32x4_t b0,
          const float32x4_t b1,
          const float32x4_t b2
        ) {
          vst1q_f32(mB + 0,  vdupq_n_f32(0.0f));
          vst1q_f32(mB + 4,  b2);
          vst1q_f32(mB + 8,  b1);
          vst1q_f32(mB + 12, b0);
        }

        float mA[16];
        float mB[16];
        float mAI[4];
    };

    enum Mode {
      MODE_12DB = 1,
      MODE_24DB = 2,
      MODE_36DB = 3
    };

    template <int STAGES>
    class Filter {
      public:
        inline Filter() {
          for (int i = 0; i < STAGES; i++) {
            mStages[i] = vdupq_n_f32(0);
          }
        }

        inline void process(
          const Coefficients& cf,
          const float32x4_t input
        ) {
          const auto a = cf.a(), b = cf.b();
          const auto ai = cf.aI();

          float xs[4];
          vst1q_f32(xs, input);

          for (int i = 0; i < 4; i++) {
            auto _ai = vdupq_n_f32(ai[i]);
            auto _b = b.val[i];
            auto _a = a.val[i];

            mStages[0] = util::four::push(mStages[0], xs[i]);

            for (int j = 1; j < STAGES; j++) {
              auto terms = _ai * ((mStages[j - 1] * _b) - (mStages[j] * _a));
              mStages[j] = util::four::push(mStages[j], util::four::sum_lanes(terms));
            }
          }
        }

        inline float32x4_t mode12dB() const { return mStages[1]; }
        inline float32x4_t mode24dB() const { return mStages[2]; }
        inline float32x4_t mode36dB() const { return mStages[3]; }

        inline float32x4_t mode(Mode m) const {
          switch (m) {
            case MODE_24DB: return mode24dB();
            case MODE_36DB: return mode36dB();
            default: return mode12dB();
          }
        }

      private:
        float32x4_t mStages[STAGES];
    };
  }
}