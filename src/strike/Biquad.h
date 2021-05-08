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
    Coefficients(float f, float q, float sr);

    void initCommon(float f, float q, float sr) {
      theta = 2.0f * M_PI * f / sr;
      st    = sin(theta);
      ct    = cos(theta);
      alpha = st / (2.0f * q);
      a0    = 1.0f + alpha;
      a0I   = 1.0f / a0;
      a1    = -2.0f * ct;
      a2    = 1.0f - alpha;
    }

    float theta, st, ct;
    float alpha, a0I;
    float a0, a1, a2;
    float b0, b1, b2;
  };

  template <>
  inline Coefficients<LOWPASS>::Coefficients(float f, float q, float sr) {
    this->initCommon(f, q, sr);
    b1 = 1.0f - ct;
    b0 = b1 / 2.0f;
    b2 = b0;
  }

  template <>
  inline Coefficients<HIGHPASS>::Coefficients(float f, float q, float sr) {
    this->initCommon(f, q, sr);
    b1 = -(1.0f + ct);
    b0 = -b1 / 2.0f;
    b2 = b0;
  }

  template <>
  inline Coefficients<BANDPASS>::Coefficients(float f, float q, float sr) {
    this->initCommon(f, q, sr);
    b0 = alpha;
    b1 = 0;
    b2 = -alpha;
  }

  struct Filter {
    template <FilterType FT>
    float process(Coefficients<FT> c, float x) {
      float y = (c.b0 * x
              + c.b1 * mXz1
              + c.b2 * mXz2
              - c.a1 * mYz1
              - c.a2 * mYz2) * c.a0I;
        
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
      inline Coefficients(const Constants &c, const float32x4_t f, const float32x4_t q);

      void initCommon(const Constants &c, const float32x4_t f, const float32x4_t q) {
        theta = f * c.twoPiOverSampleRate;
        simd_sincos(theta, &st, &ct);

        auto qI = simd_invert(q);

        alpha = st * c.half * qI;
        a0    = c.one + alpha;
        a0I   = simd_invert(a0);
        a1    = c.negTwo * ct;
        a2    = c.one - alpha;
      }

      float32x4_t theta, st, ct;
      float32x4_t alpha, a0I;
      float32x4_t a0, a1, a2;
      float32x4_t b0, b1, b2;
    };

    template <>
    inline Coefficients<LOWPASS>::Coefficients(const Constants &c, const float32x4_t f, const float32x4_t q) {
      this->initCommon(c, f, q);
      b1 = c.one - ct;
      b0 = b1 * c.half;
      b2 = b0;
    }

    template <>
    inline Coefficients<HIGHPASS>::Coefficients(const Constants &c, const float32x4_t f, const float32x4_t q) {
      this->initCommon(c, f, q);
      b1 = c.negOne - ct;
      b0 = b1 * c.negHalf;
      b2 = b0;
    }

    template <>
    inline Coefficients<BANDPASS>::Coefficients(const Constants &c, const float32x4_t f, const float32x4_t q) {
      this->initCommon(c, f, q);
      b0 = alpha;
      b1 = c.zero;
      b2 = alpha * c.negOne;
    }

    struct Filter {
      inline Filter() {
        x = y = vdupq_n_f32(0.0f);
      }

      template <FilterType FT>
      inline float32x4_t process(const Constants &c, const Coefficients<FT> &cf, float32x4_t input) {
        float32x4_t a[4], b[4];
        util::simd::rtocq_p_f32(a, cf.a1, cf.a2, c.zeroX2);
        util::simd::rtocq_f32(b, cf.b0, cf.b1, cf.b2, c.zero);

        float xs[4], a0I[4];
        vst1q_f32(xs, input);
        vst1q_f32(a0I, cf.a0I);

        for (int i = 0; i < 4; i++) {
          x = util::simd::pushq_f32(x, xs[i]);

          float32x4_t aI = vdupq_n_f32(a0I[i]);
          float32x4_t terms = aI * ((b[i] * x) - (a[i] * y));

          float _y = util::simd::sumq_f32(terms);
          y = util::simd::pushq_f32(y, _y);
        }

        return util::simd::revq_f32(y);
      }

      float32x4_t x, y;
    };
  }
}