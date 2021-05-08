#pragma once

#include <math.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace biquad {
  enum FilterType {
    LOWPASS,
    HIGHPASS,
    BANDPASS
  };

  // https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
  // https://arachnoid.com/BiQuadDesigner/index.html
  template <FilterType FT>
  struct Coefficients {
    inline Coefficients(float f, float q, float sr) {
      theta = 2.0f * M_PI * f / sr;
      st    = sin(theta);
      ct    = cos(theta);
      alpha = st / (2.0f * q);
      a0    = 1.0f + alpha;
      a0I   = 1.0f / a0;
      a1    = -2.0f * ct;
      a2    = 1.0f - alpha;
    }

    inline void initType();

    float theta, st, ct;
    float alpha, a0I;
    float a0, a1, a2;
    float b0, b1, b2;
  };

  template <>
  inline void Coefficients<LOWPASS>::initType() {
    b1 = 1.0f - ct;
    b0 = b1 / 2.0f;
    b2 = b0;
  }

  template <>
  inline void Coefficients<HIGHPASS>::initType() {
    b1 = -(1.0f + ct);
    b0 = -b1 / 2.0f;
    b2 = b0;
  }

  template <>
  inline void Coefficients<BANDPASS>::initType() {
    b0 = alpha;
    b1 = 0;
    b2 = -alpha;
  }

  struct Filter {
    template <FilterType FT>
    float process(const Coefficients<FT> &cf, float x) {
      float y = (cf.b0 * x
              + cf.b1 * mXz1
              + cf.b2 * mXz2
              - cf.a1 * mYz1
              - cf.a2 * mYz2) * cf.a0I;
        
      mXz2 = mXz1;
      mXz1 = x;
      mYz2 = mYz1;
      mYz1 = y;

      return y;
    }

    float mXz1 = 0, mXz2 = 0;
    float mYz1 = 0, mYz2 = 0;
  };

  namespace simd {
    struct Constants {
      inline Constants(int sampleRate) {
        negTwo  = vdupq_n_f32(-2.0f);
        negOne  = vdupq_n_f32(-1.0f);
        negHalf = vdupq_n_f32(-0.5f);
        zeroX2  = vdup_n_f32(0.0f);
        zero    = vdupq_n_f32(0.0f);
        half    = vdupq_n_f32(0.5f);
        one     = vdupq_n_f32(1.0f);
        twoPiOverSampleRate = vdupq_n_f32((2.0f * M_PI) / (float)sampleRate);
      }

      float32x4_t negTwo;
      float32x4_t negOne;
      float32x4_t negHalf;
      float32x2_t zeroX2;
      float32x4_t zero;
      float32x4_t half;
      float32x4_t one;
      float32x4_t twoPiOverSampleRate;
    };

    template <FilterType FT>
    struct Coefficients {
      inline Coefficients(const Constants &c, const float32x4_t f, const float32x4_t q) {
        theta = f * c.twoPiOverSampleRate;
        simd_sincos(theta, &st, &ct);

        auto qI = simd_invert(q);

        alpha = st * c.half * qI;
        a0    = c.one + alpha;
        a0I   = simd_invert(a0);
        a1    = c.negTwo * ct;
        a2    = c.one - alpha;

        initType(c);

        util::simd::rtocq_f32(a, a1, a2, c.zero, c.zero);
        util::simd::rtocq_f32(b, b0, b1, b2, c.zero);

        float _aI[4];
        vst1q_f32(_aI, a0I);
        for (int i = 0; i < 4; i++)
          aI[i] = vdupq_n_f32(_aI[i]);
      }

      inline void initType(const Constants &c);

      float32x4_t theta, st, ct;
      float32x4_t alpha, a0I;
      float32x4_t a0, a1, a2;
      float32x4_t b0, b1, b2;

      float32x4_t a[4], b[4], aI[4];
    };

    template <>
    inline void Coefficients<LOWPASS>::initType(const Constants &c) {
      b1 = c.one - ct;
      b0 = b1 * c.half;
      b2 = b0;
    }

    template <>
    inline void Coefficients<HIGHPASS>::initType(const Constants &c) {
      b1 = c.negOne - ct;
      b0 = b1 * c.negHalf;
      b2 = b0;
    }

    template <>
    inline void Coefficients<BANDPASS>::initType(const Constants &c) {
      b0 = alpha;
      b1 = c.zero;
      b2 = alpha * c.negOne;
    }

    struct Filter {
      inline Filter() {
        x = y = vdupq_n_f32(0.0f);
      }

      template <FilterType FT>
      inline float32x4_t process(const Coefficients<FT> &cf, float32x4_t input) {
        float xs[4];
        vst1q_f32(xs, input);
        return processArray(cf, xs);
      }

      template <FilterType FT>
      inline float32x4_t processArray(const Coefficients<FT> &cf, const float *xs) {
        for (int i = 0; i < 4; i++) {
          x = util::simd::pushq_f32(x, xs[i]);

          float32x4_t terms = cf.aI[i] * vmlsq_f32(cf.b[i] * x, cf.a[i], y);
          float _y = util::simd::sumq_f32(terms);

          y = util::simd::pushq_f32(y, _y);
        }

        return util::simd::revq_f32(y);
      }

      float32x4_t x, y;
    };
  }
}