#pragma once

#include <math.h>
#include <hal/constants.h>
#include <hal/simd.h>
#include <hal/neon.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace util {
  namespace simd {
    struct clamp {
      float32x4_t min, max;

      inline clamp(float _min, float _max) {
        min = vdupq_n_f32(_min);
        max = vdupq_n_f32(_max);
      }

      inline clamp(const float32x4_t _min, const float32x4_t _max) {
        min = _min;
        max = _max;
      }

      inline float32x4_t process(const float32x4_t in) {
        return vmaxq_f32(min, vminq_f32(max, in));
      }
    };

    struct vpo {
      float32x4_t glog2;

      inline vpo() {
        glog2 = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
      }

      inline float32x4_t process(const float32x4_t vpo, const float32x4_t f0) {
        return f0 * simd_exp(vpo * glog2);
      }
    };

    struct tanh {
      float32x4_t c1, c2, c3, c4, c5, c6;

      inline tanh() {
        c1 = vdupq_n_f32(378);
        c2 = vdupq_n_f32(17325);
        c3 = vdupq_n_f32(135135);
        c4 = vdupq_n_f32(28);
        c5 = vdupq_n_f32(3150);
        c6 = vdupq_n_f32(62370);
      }

      // tanh approximation (neon w/ division via newton's method)
      // https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
      inline float32x4_t process(float32x4_t in) {
        float32x4_t x, x2, a, b, invb;

        x  = in;
        x2 = x * x;

        a = vmlaq_f32(c2, x2, x2 + c1);
        a = vmlaq_f32(c3, x2, a) * x;

        b = vmlaq_f32(c5, x2, c4);
        b = vmlaq_f32(c6, x2, b);
        b = vmlaq_f32(c3, x2, b);

        invb = simd_invert(b);
        return a * invb;
      }
    };

    // Push a new value into the front of the vector.
    inline float32x4_t pushq_f32(const float32x4_t current, const float next) {
      float32x4_t t = vtrnq_f32(current, current).val[0];
      float32x4_t z = vzipq_f32(current, current).val[0];
      float32x4_t s = vtrnq_f32(z, t).val[0];
      return vsetq_lane_f32(next, s, 0);
    }

    // Add all lanes together.
    inline float sumq_f32(const float32x4_t v) {
      auto pair = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
      pair = vpadd_f32(pair, pair);
      return vget_lane_f32(pair, 0);
    }

    // Reverse the lane order.
    inline float32x4_t revq_f32(const float32x4_t v) {
      auto pair = vrev64q_f32(v);
      return vcombine_f32(vget_high_f32(pair), vget_low_f32(pair));
    }

    // See: rtocq_f32
    // A partial version. Same principle, but optimized for two rows using the
    // rem value to fill in the ends.
    inline void rtocq_p_f32(
      float32x4_t* out,
      const float32x4_t a,
      const float32x4_t b,
      const float32x2_t rem
    ) {
      float32x4x2_t ab = vzipq_f32(a, b);

      out[0] = vcombine_f32( vget_low_f32(ab.val[0]), rem);
      out[1] = vcombine_f32(vget_high_f32(ab.val[0]), rem);
      out[2] = vcombine_f32( vget_low_f32(ab.val[1]), rem);
      out[3] = vcombine_f32(vget_high_f32(ab.val[1]), rem);
    }

    // Consider the four input vectors as rows in a matrix and create four new
    // vectors from the columns of that matrix.
    inline void rtocq_f32(
      float32x4_t* out,
      const float32x4_t a,
      const float32x4_t b,
      const float32x4_t c,
      const float32x4_t d
    ) {
      float32x4x2_t ab = vzipq_f32(a, b);
      float32x4x2_t cd = vzipq_f32(c, d);

      out[0] = vcombine_f32( vget_low_f32(ab.val[0]),  vget_low_f32(cd.val[0]));
      out[1] = vcombine_f32(vget_high_f32(ab.val[0]), vget_high_f32(cd.val[0]));
      out[2] = vcombine_f32( vget_low_f32(ab.val[1]),  vget_low_f32(cd.val[1]));
      out[3] = vcombine_f32(vget_high_f32(ab.val[1]), vget_high_f32(cd.val[1]));
    }
  }

  // tanh approximation
  // https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
  inline float tanh(float in) {
    float x, x2, a, b;

    x  = in;
    x2 = x * x;

    a = (((x2 + 378) * x2 + 17325) * x2 + 135135) * x;
    b = ((28 * x2 + 3150) * x2 + 62370) * x2 + 135135;

    return a / b;
  }
}