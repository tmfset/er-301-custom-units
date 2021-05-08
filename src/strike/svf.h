#pragma once

#include <math.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>

namespace svf {
  enum FilterType {
    LOWPASS,
    HIGHPASS,
    BANDPASS
  };

  namespace simd {
    struct Constants {
      inline Constants(int sampleRate) {
        one = vdupq_n_f32(1.0f);
        piOverSampleRate = vdupq_n_f32(M_PI / (float)sampleRate);
      }

      float32x4_t one;
      float32x4_t piOverSampleRate;
    };

    struct Coefficients {
      inline Coefficients(const Constants &c, const float32x4_t f, const float32x4_t q) {
        auto _g = util::simd::tanq_f32(f * c.piOverSampleRate);
        auto _k = simd_invert(q);

        auto _a1 = simd_invert(c.one + _g * (_g + _k));
        auto _a2 = _g * _a1;
        auto _a3 = _g * _a2;

        vst1q_f32(g,  _g);
        vst1q_f32(k,  _k);
        vst1q_f32(a1, _a1);
        vst1q_f32(a2, _a2);
        vst1q_f32(a3, _a3);
      }

      float g[4], k[4];
      float a1[4], a2[4], a3[4];
    };

    struct Filter {
      inline Filter() {
        ic1eq = ic2eq = 0;
      }

      inline float32x4_t process(const Coefficients &cf, const float32x4_t input) {
        float xs[4];
        vst1q_f32(xs, input);
        return processArray(cf, xs);
      }

      inline float32x4_t processArray(const Coefficients &cf, const float *xs) {
        float out[4];
        for (int i = 0; i < 4; i++) {
          float v3 = xs[i] - ic2eq;
          float v1 = cf.a1[i] * ic1eq + cf.a2[i] * v3;
          float v2 = ic2eq + cf.a2[i] * ic1eq + cf.a3[i] * v3;

          ic1eq = 2.0f * v1 - ic1eq;
          ic2eq = 2.0f * v2 - ic2eq;

          out[i] = v2;
        }

        return vld1q_f32(out);
      }

      float ic1eq, ic2eq;
    };
  }
}