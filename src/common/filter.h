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

  namespace svf {
    namespace two {
      struct Coefficients {
        inline void configure(
          const float32x2_t f0,
          const float32x2_t vpo,
          const float32x2_t q
        ) {
          mK = util::two::invert(util::two::exp_ns_f32(q, 0.70710678118f, 1000.0f));

          auto f = util::two::vpo_scale_limited(f0, vpo);
          auto g = util::two::tan(f * vdup_n_f32(M_PI * globalConfig.samplePeriod));
          mA1 = util::two::invert(vmla_f32(vdup_n_f32(1), g, vadd_f32(g, mK)));
          mA2 = vmul_f32(g, mA1);
          mA3 = vmul_f32(g, mA2);
        }

        float32x2_t mA1 = vdup_n_f32(0);
        float32x2_t mA2 = vdup_n_f32(0);
        float32x2_t mA3 = vdup_n_f32(0);
        float32x2_t mK  = vdup_n_f32(0);
      };
    }

    namespace four {
      struct Coefficients {
        inline void configure(float32x4_t f0, float32x4_t q) {
          mK = util::simd::invert(q);

          auto g = util::simd::tan(f0 * vdupq_n_f32(M_PI * globalConfig.samplePeriod));
          mA1 = util::simd::invert(vmlaq_f32(vdupq_n_f32(1.0f), g, g + mK));
          mA2 = g * mA1;
          mA3 = g * mA2;
        }

        float32x4_t mA1 = vdupq_n_f32(0);
        float32x4_t mA2 = vdupq_n_f32(0);
        float32x4_t mA3 = vdupq_n_f32(0);
        float32x4_t mK = vdupq_n_f32(0);
      };

      struct Lowpass {
        inline void configure(float32x4_t f0, float32x4_t q) {
          mCf.configure(f0, q);
        }

        inline float32x4_t process(
          const float32x4_t x
        ) {
          auto v3 = x - mIc2;
          auto v1 = mCf.mA1 * mIc1 + mCf.mA2 * v3;
          auto v2 = mIc2 + mCf.mA2 * mIc1 + mCf.mA3 * v3;

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
          auto v1 = cf.mA1 * mIc1 + cf.mA2 * v3;
          auto v2 = mIc2 + cf.mA2 * mIc1 + cf.mA3 * v3;

          auto two = vdupq_n_f32(2);
          mIc1 = two * v1 - mIc1;
          mIc2 = two * v2 - mIc1;

          mLp = v2;
          mBp = v1;
          mHp = x - cf.mK * mBp - mLp;
        }

        float32x4_t mLp;
        float32x4_t mBp;
        float32x4_t mHp;

        float32x4_t mIc1 = vdupq_n_f32(0);
        float32x4_t mIc2 = vdupq_n_f32(0);
      };
    }

    class Coefficients {
      public:
        inline void update(
          const float32x4_t vpo,
          const float32x4_t f0,
          const float32x4_t _q
        ) {
          auto f = util::simd::vpo_scale(vpo, f0);
          auto q = util::simd::exp_n_scale(_q, 0.70710678118f, 1000.0f);

          const auto g = util::simd::tan(f * vdupq_n_f32(M_PI * globalConfig.samplePeriod));
          const auto k = util::simd::invert(q);
          vst1q_f32(mK, k);

          const auto a1 = util::simd::invert(vmlaq_f32(vdupq_n_f32(1.0f), g, g + k));
          const auto a2 = g * a1;
          const auto a3 = g * a2;
          vst1q_f32(mA1223 + 0, a1);
          vst1q_f32(mA1223 + 4, a2);
          vst1q_f32(mA1223 + 8, a2);
          vst1q_f32(mA1223 + 12, a3);
        }

        inline float32x4x4_t a1223() const { return vld4q_f32(mA1223); }
        inline float32x4_t   k()     const { return vld1q_f32(mK); }

      private:
        float mA1223[16];
        float mK[4];
    };

    class Mix {
      public:
        inline void update(const float32x4_t m) {
          const auto half = vdupq_n_f32(0.5);
          vst1q_f32(mDst, util::simd::twice(vabdq_f32(m, half)));
          vst1q_f32(mClm, vcvtq_n_f32_u32(vcltq_f32(m, half), 32));
        }

        inline float32x4_t dst() const { return vld1q_f32(mDst); }
        inline float32x4_t clm() const { return vld1q_f32(mClm); }

      private:
        float mDst[4];
        float mClm[4];
    };

    class Filter {
      public:
        inline Filter() {
          mIceq[0] = 0;
          mIceq[1] = 0;
        }

        inline void process(
          const Coefficients &cf,
          const float32x4_t input
        ) {
          // Reference:
          //   https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
          //   https://github.com/FredAntonCorvest/Common-DSP
          //
          // Heavily optimized to do all operations in parallel. Tested by
          // examining the assembly.
          //
          // Here's the value matrix derived from the original sources:
          //
          //        t1      t3          t2      t4
          // v1: + a1[i] * ic1eq     + a2[i] * xs[i]
          //     - a2[i] * ic2eq     + 0     * ic2eq
          //
          // v2: + a2[i] * ic1eq     + a3[i] * xs[i]
          //     - a3[i] * ic2eq     + 1     * ic2eq
          //
          // Or in plain maths:
          //
          //   v3 = xs[i] - ic2eq
          //   v1 = a1[i] * ic1eq + a2[i] * v3
          //   v2 = ic2eq + a2[i] * ic1eq + a3[i] * v3
          //
          //   ic1eq = 2.0f * v1 - ic1eq
          //   ic2eq = 2.0f * v2 - ic2eq
          //
          // Note that v3 is inlined and terms are rearranged to reach
          // the above matrix.

          const auto a1223 = cf.a1223();
          const auto mt1 = vtrnq_f32(vdupq_n_f32(1), vdupq_n_f32(-1)).val[0];//util::simd::makeq_f32(1.0f, -1.0f, 1.0f, -1.0f);
          const auto mt2 = vcombine_f32(vdup_n_f32(0), vdup_n_f32(1));//util::simd::makeq_f32(0.0f, 0.0f, 1.0f, 1.0f);

          auto iceq = vld1_f32(mIceq);

          float xs[4], lp[4], bp[4];
          vst1q_f32(xs, input);
          for (int i = 0; i < 4; i++) {
            auto t1 = a1223.val[i] * mt1;
            auto t2 = vtrnq_f32(a1223.val[i], mt2).val[1];
            auto t3 = vcombine_f32(iceq, iceq);
            auto t4 = vtrnq_f32(vdupq_n_f32(xs[i]), t3).val[1];

            auto vp  = vmlaq_f32(vmulq_f32(t1, t3), t2, t4);
            auto v1  = util::simd::padd_self(vget_low_f32(vp));
            auto v2  = util::simd::padd_self(vget_high_f32(vp));
            auto v12 = vtrn_f32(v1, v2).val[0];

            iceq = vsub_f32(vadd_f32(v12, v12), iceq);

            bp[i] = vget_lane_f32(v12, 0);
            lp[i] = vget_lane_f32(v12, 1);
          }

          auto _lp = vld1q_f32(lp);
          auto _bp = vld1q_f32(bp);
          auto _hp = vmlsq_f32(input, cf.k(), _bp) - _lp;

          vst1_f32(mIceq, iceq);
          vst1q_f32(mLp, _lp);
          vst1q_f32(mBp, _bp);
          vst1q_f32(mHp, _hp);
        }

        inline float32x4_t lowpass()  const { return vld1q_f32(mLp); }
        inline float32x4_t bandpass() const { return vld1q_f32(mBp); }
        inline float32x4_t highpass() const { return vld1q_f32(mHp); }

        inline float32x4_t mix(const Mix &mix) const {
          auto dst = mix.dst();
          auto clm = mix.clm();
          auto bp  = bandpass();

          auto bpLvl = vmlsq_f32(bp,    dst, bp);
          auto lpLvl = vmlaq_f32(bpLvl, dst, lowpass());
          auto hpLvl = vmlaq_f32(bpLvl, dst, highpass());

          return util::simd::lerp(hpLvl, lpLvl, clm);
        }

      private:
        float mIceq[2];
        float mLp[4];
        float mBp[4];
        float mHp[4];
    };
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
          mA0I = util::simd::invert(one + a);
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
          auto f = util::simd::vpo_scale(vpo, f0);
          auto q = util::simd::exp_n_scale(_q, 0.70710678118f, 500.0f);

          auto theta = f * vdupq_n_f32(2.0f * M_PI * globalConfig.samplePeriod);
          float32x4_t sinTheta, cosTheta;
          util::simd::sincos_f32(theta, &sinTheta, &cosTheta);

          auto qI = util::simd::invert(q);

          auto one  = vdupq_n_f32(1.0f);
          auto sh = sinTheta * vdupq_n_f32(0.5f);
          auto alpha = sh * qI;
          auto a0 = one + alpha;
          auto a1 = vnegq_f32(vdupq_n_f32(2.0f) * cosTheta);
          auto a2 = one - alpha;
          writeA(a1, a2);

          auto a0I = util::simd::invert(a0);
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

            mStages[0] = util::simd::pushq_f32(mStages[0], xs[i]);

            for (int j = 1; j < STAGES; j++) {
              auto terms = _ai * ((mStages[j - 1] * _b) - (mStages[j] * _a));
              mStages[j] = util::simd::pushq_f32(mStages[j], util::simd::sumq_f32(terms));
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