#pragma once

#include <hal/simd.h>
#include <hal/neon.h>

namespace strike {
  struct simd_tanh_constants {
    float32x4_t c1, c2, c3, c4, c5, c6;

    inline simd_tanh_constants() {
      c1 = vdupq_n_f32(378);
      c2 = vdupq_n_f32(17325);
      c3 = vdupq_n_f32(135135);
      c4 = vdupq_n_f32(28);
      c5 = vdupq_n_f32(3150);
      c6 = vdupq_n_f32(62370);
    }
  };

  // tanh approximation (neon w/ division via newton's method)
  // https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
  inline float32x4_t simd_tanh(float32x4_t in, const simd_tanh_constants &c) {
    float32x4_t x, x2, a, b, invb;

    x  = in;
    x2 = x * x;

    a = vmlaq_f32(c.c2, x2, x2 + c.c1);
    a = vmlaq_f32(c.c3, x2, a) * x;

    b = vmlaq_f32(c.c5, x2, c.c4);
    b = vmlaq_f32(c.c6, x2, b);
    b = vmlaq_f32(c.c3, x2, b);

    invb = simd_invert(b);
    return a * invb;
  }

  struct simd_constants {
    float32x4_t negTwo;
    float32x4_t negOne;
    float32x2_t zeroh;
    float32x4_t zero;
    float32x4_t half;
    float32x4_t one;
    float32x4_t two;

    inline simd_constants() {
      negTwo = vdupq_n_f32(-2.0f);
      negOne = vdupq_n_f32(-1.0f);
      zeroh  = vdup_n_f32(0.0f);
      zero   = vdupq_n_f32(0.0f);
      half   = vdupq_n_f32(0.5f);
      one    = vdupq_n_f32(1.0f);
      two    = vdupq_n_f32(2.0f);
    }
  };

  // https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
  struct simd_biquad_coefficients {
    float32x4_t beta0, beta1, beta2;
    float32x4_t alpha0, alpha1, alpha2;
    float32x4_t alpha0inv;

    inline simd_biquad_coefficients(float32x4_t f, float32x4_t q, const simd_constants &c) {
      float32x4_t qinv  = simd_invert(q);
      float32x4_t theta = f;

      float32x4_t sinTheta, cosTheta;
      simd_sincos(theta, &sinTheta, &cosTheta);

      beta1 = c.one - cosTheta;
      beta0 = beta1 * c.half;
      beta2 = beta0;

      float32x4_t alpha = sinTheta * c.half * qinv;

      alpha0 = c.one + alpha;
      alpha0inv = simd_invert(alpha0);
      alpha0 = c.negOne * alpha0; // Negated

      alpha1 = c.two * cosTheta; // Negated
      alpha2 = alpha - c.one; // Negated
    }
  };

  inline float32x4_t push_lane_f32(float32x4_t current, float next) {
    float32x4_t t = vtrnq_f32(current, current).val[0];
    float32x4_t z = vzipq_f32(current, current).val[0];
    float32x4_t s = vtrnq_f32(z, t).val[0];
    return vsetq_lane_f32(next, s, 0);
  }

  struct simd_biquad {
    float32x4_t in, out;

    inline simd_biquad() {
      in  = vdupq_n_f32(0.0f);
      out = vdupq_n_f32(0.0f);
    }


    float32x4_t process(
      float *buffer,
      int offset,
      const simd_biquad_coefficients& cf,
      const simd_constants &c
    ) {
      float32x4x2_t bleft  = vzipq_f32(cf.beta0, cf.beta1);
      float32x4x2_t bright = vzipq_f32(cf.beta2, c.zero);
      float32x4x2_t aleft  = vzipq_f32(cf.alpha1, cf.alpha2);

      float32x4_t b[4], a[4];
      b[0] = vcombine_f32(vget_low_f32(bleft.val[0]), vget_low_f32(bright.val[0]));
      b[1] = vcombine_f32(vget_high_f32(bleft.val[0]), vget_high_f32(bright.val[0]));
      b[2] = vcombine_f32(vget_low_f32(bleft.val[1]), vget_low_f32(bright.val[1]));
      b[3] = vcombine_f32(vget_high_f32(bleft.val[1]), vget_high_f32(bright.val[1]));

      a[0] = vcombine_f32(vget_low_f32(aleft.val[0]), c.zeroh);
      a[1] = vcombine_f32(vget_high_f32(aleft.val[0]), c.zeroh);
      a[2] = vcombine_f32(vget_low_f32(aleft.val[1]), c.zeroh);
      a[3] = vcombine_f32(vget_high_f32(aleft.val[1]), c.zeroh);

      for (int i = 0; i < 4; i++) {
        float next = buffer[offset + i];
        in = push_lane_f32(in, next);

        float32x4_t ai = vdupq_n_f32(vgetq_lane_f32(cf.alpha0inv, i));

        float32x4_t s2 = vmlaq_f32(a[i] * in, b[i], out
        float32x4_t s = ai * vmlaq_f32(b[i] * in, a[i], out);
        float32x2_t t = vpadd_f32(vget_low_f32(s), vget_high_f32(s));
        t = vpadd_f32(t, t);

        float r = vget_lane_f32(t, 0);
        out = push_lane_f32(out, r);
      }

      // for (int i = 0; i < 4; i++) {
      //   float next = buffer[offset + i];
      //   in = simd_push_lane(in, next);

      //   float32x4_t b = c.beta0;
      //   b = vsetq_lane_f32(vgetq_lane_f32(c.beta0, i), b, 0);
      //   b = vsetq_lane_f32(vgetq_lane_f32(c.beta1, i), b, 1);
      //   b = vsetq_lane_f32(vgetq_lane_f32(c.beta2, i), b, 2);
      //   b = vsetq_lane_f32(0.0f, b, 3);

      //   float32x4_t a = c.alpha0;
      //   a = vsetq_lane_f32(-vgetq_lane_f32(c.alpha1, i), a, 0);
      //   a = vsetq_lane_f32(-vgetq_lane_f32(c.alpha2, i), a, 1);
      //   a = vsetq_lane_f32(0.0f, a, 2);
      //   a = vsetq_lane_f32(0.0f, a, 3);

      //   float32x4_t ai = vdupq_n_f32(vgetq_lane_f32(c.alpha0inv, i));

      //   float32x4_t s = ai * vmlaq_f32(b * in, a, out);
      //   float32x2_t t = vpadd_f32(vget_low_f32(s), vget_high_f32(s));
      //   t = vpadd_f32(t, t);

      //   float r = vget_lane_f32(t, 0);
      //   out = simd_push_lane(out, r);
      // }

      return out;
    }
  };

  struct sv_filter {
    float32x4_t in, out;

    sv_filter() {
      in  = vdupq_n_f32(0.0f);
      out = vdupq_n_f32(0.0f);
    }

  };
}