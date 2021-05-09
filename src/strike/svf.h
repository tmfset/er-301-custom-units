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
        g = util::simd::tan(f * c.piOverSampleRate);
        k = util::simd::invert(q);

        a1 = util::simd::invert(vmlaq_f32(c.one, g, g + k));
        a2 = g * a1;
        a3 = g * a2;
      }

      float32x4_t g, k;
      float32x4_t a1, a2, a3;
    };

    struct Filter {
      inline Filter() {
        iceq = vdup_n_f32(0);
      }

      inline float32x4_t process(const Coefficients &cf, const float32x4_t input) {
        float xs[4];
        vst1q_f32(xs, input);
        return processArray(cf, xs);
      }

      inline float32x4_t processArray(const Coefficients &cf, const float *xs) {
        // Reference:
        //   https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
        //   https://github.com/FredAntonCorvest/Common-DSP
        //
        // Heavily optimized to do all operations in parallel. Tested by
        // examining the assembly, the entire loop is inlined for excellent
        // speed.
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

        float32x4_t a1223[4];
        util::simd::rtocq_f32(a1223, cf.a1, cf.a2, cf.a2, cf.a3);
        auto mt1 = util::simd::makeq_f32(1, -1, 1, -1);
        auto mt2 = util::simd::makeq_f32(0, 0, 1, 1);

        float32x2_t _iceq = iceq;

        float out[4];
        for (int i = 0; i < 4; i++) {
          auto t1 = a1223[i] * mt1;
          auto t2 = vtrnq_f32(a1223[i], mt2).val[1];
          auto t3 = vcombine_f32(_iceq, _iceq);
          auto t4 = vtrnq_f32(vdupq_n_f32(xs[i]), t3).val[1];

          auto v1v2 = t1 * t3 + t2 * t4;
          auto v1  = util::simd::padd_self(vget_low_f32(v1v2));
          auto v2  = util::simd::padd_self(vget_high_f32(v1v2));

          _iceq = vsub_f32(vmul_n_f32(vtrn_f32(v1, v2).val[0], 2), _iceq);

          out[i] = vget_lane_f32(v2, 0);
        }

        iceq = _iceq;

        return vld1q_f32(out);
      }

      float32x2_t iceq;
    };
  }
}