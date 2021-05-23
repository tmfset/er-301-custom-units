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
    inline float32x4_t invert(const float32x4_t x) {
      float32x4_t inv;
      // https://en.wikipedia.org/wiki/Division_algorithm#Newton.E2.80.93Raphson_division
      inv = vrecpeq_f32(x);
      // iterate 3 times for 24 bits of precision
      inv *= vrecpsq_f32(x, inv);
      inv *= vrecpsq_f32(x, inv);
      inv *= vrecpsq_f32(x, inv);
      return inv;
    }

    inline float32x4_t lnot(const float32x4_t x) {
      return vdupq_n_f32(1) - x;
    }

    struct clamp_low {
      const float32x4_t min;

      inline clamp_low(float _min) :
        min(vdupq_n_f32(_min)) {}

      inline float32x4_t low(const float32x4_t in) const {
        return vmaxq_f32(min, in);
      }

      inline float32x4_t lowBase(const float32x4_t in) const {
        return low(in + min);
      }
    };

    struct clamp {
      const float32x4_t min, max;

      inline clamp(float _min, float _max) :
        min(vdupq_n_f32(_min)),
        max(vdupq_n_f32(_max)) { }

      inline clamp(const float32x4_t _min, const float32x4_t _max):
        min(_min),
        max(_max) { }

      inline float32x4_t process(const float32x4_t in) const {
        return vminq_f32(max, vmaxq_f32(min, in));
      }
    };

    inline float32x4_t clamp_unit(const float32x4_t x) {
      return vminq_f32(vdupq_n_f32(1), vmaxq_f32(vdupq_n_f32(-1), x));
    }

    inline float32x4_t clamp_punit(const float32x4_t x) {
      return vminq_f32(vdupq_n_f32(1), vmaxq_f32(vdupq_n_f32(0), x));
    }

    inline float32x4_t twice(const float32x4_t x) {
      return x + x;
    }

    inline float32x4_t floor(const float32x4_t x) {
      return vcvtq_f32_s32(vcvtq_s32_f32(x));
    }

    // tanh approximation (neon w/ division via newton's method)
    // https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
    inline float32x4_t tanh(const float32x4_t in) {
      float32x4_t x, x2, a, b;

      x  = in;
      x2 = x * x;

      a = vmlaq_f32(vdupq_n_f32(17325), x2, x2 + vdupq_n_f32(378));
      a = vmlaq_f32(vdupq_n_f32(135135), x2, a) * x;

      b = vmlaq_f32(vdupq_n_f32(3150), x2, vdupq_n_f32(28));
      b = vmlaq_f32(vdupq_n_f32(62370), x2, b);
      b = vmlaq_f32(vdupq_n_f32(135135), x2, b);

      return a * invert(b);;
    }

    inline float32x4_t tan(const float32x4_t f) {
      float32x4_t sin, cos;
      simd_sincos(f, &sin, &cos);
      return sin * invert(cos);
    }

    const float32x4_t vpoLogMax = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
    const clamp vpoFreqClamp { 1, (float)(globalConfig.sampleRate / 4) };

    inline float32x4_t vpo_scale(
      const float32x4_t vpo,
      const float32x4_t f0
    ) {
      return vpoFreqClamp.process(f0 * simd_exp(vpo * vpoLogMax));
    }

    inline float32x4_t lerp(const float32x4_t from, const float32x4_t to, const float32x4_t by) {
      return vmlaq_f32(vmlsq_f32(from, from, by), to, by);
    }

    inline float32x4_t fcgt(const float32x4_t x, const float32x4_t v) {
      return vcvtq_n_f32_u32(vcgtq_f32(x, v), 32);
    }

    inline float32x4_t fclt(const float32x4_t x, const float32x4_t v) {
      return vcvtq_n_f32_u32(vcltq_f32(x, v), 32);
    }

    inline float32x4_t cgtqz_f32(const float32x4_t x) {
      return fcgt(x, vdupq_n_f32(0));
    }

    inline float32x4_t magnitude(const float32x4_t x) {
      return vabdq_f32(x, vdupq_n_f32(0));
    }

    inline void cgt_as_bool(bool *out, const float32x4_t x, const float32x4_t v) {
      uint32_t _h[4];
      vst1q_u32(_h, vcgtq_f32(x, v));
      for (int i = 0; i < 4; i++) {
        out[i] = _h[i];
      }
    }

    struct exp_scale {
      const float32x4_t min, logMin, logMax;
      const clamp xClp { 0.0, 1.0 };

      inline exp_scale(float _min, float _max) :
        min(vdupq_n_f32(_min)),
        logMin(vdupq_n_f32(logf(_min))),
        logMax(vdupq_n_f32(logf(_max))) { }

      inline float32x4_t process(const float32x4_t x) const {
        return simd_exp(lerp(logMin, logMax, xClp.process(x)));
      }

      inline float32x4_t processBase(const float32x4_t x) const {
        return process(x) - min;
      }
    };

    inline float32x4_t exp_unit_scale(const float32x4_t x, float degree) {
      auto d = vdupq_n_f32(degree);
      auto ld = vdupq_n_f32(logf(degree));
      //auto z = vdupq_n_f32(0);
      // vmlaq_f32(vmlsq_f32(from, from, by), to, by)
      return simd_exp(vmlsq_f32(ld, ld, x)) - d;
    }

    inline float32x2_t make_f32(float a, float b) {
      float x[2];
      x[0] = a;
      x[1] = b;
      return vld1_f32(x);
    }

    inline float32x4_t makeq_f32(float a, float b, float c, float d) {
      float x[4];
      x[0] = a;
      x[1] = b;
      x[2] = c;
      x[3] = d;
      return vld1q_f32(x);
    }

    inline float32x2_t padd_self(const float32x2_t v) {
      return vpadd_f32(v, v);
    }

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

  struct latch {
    inline bool readTrigger(bool high) {
      if (high) { mTrigger = mEnable; mEnable = false; }
      else      { mTrigger = false;   mEnable = true; }
      return mTrigger;
    }

    inline void readTriggers(bool *out, const float32x4_t high) {
      uint32_t _h[4];
      vst1q_u32(_h, vcgtq_f32(high, vdupq_n_f32(0)));
      for (int i = 0; i < 4; i++) {
        out[i] = readTrigger(_h[i]);
      }
    }

    bool mEnable  = true;
    bool mTrigger = false;
  };

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

  struct exp_scale {
    const float min, max;
    const float logMin, logMax;

    inline exp_scale(float _min, float _max) :
      min(_min),
      max(_max),
      logMin(logf(min)),
      logMax(logf(max + min)) { }

    inline float process(const float x) const {
      return expf(x * logMax + (1 - x) * logMin);
    }

    inline float processBase(const float x) const {
      return expf(x * logMax + (1 - x) * logMin) - min;
    }
  };
}