#pragma once

#include <od/config.h>
#include <math.h>
#include <hal/constants.h>
#include <hal/simd.h>
#include <hal/neon.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace util {
  namespace simd {
    inline float32x4_t exp_f32(const float32x4_t _x) {
#ifdef USE_SSE4
    return simd_exp(_x);
#else
      auto x = _x;
      x = vminq_f32(x, vdupq_n_f32(88.3762626647949f));
      x = vmaxq_f32(x, vdupq_n_f32(-88.3762626647949f));

      /* express exp(x) as exp(g + n*log(2)) */
      auto fx = vmlaq_f32(vdupq_n_f32(0.5f), x, vdupq_n_f32(1.44269504088896341f));

      /* perform a floorf */
      auto tmp = vcvtq_f32_s32(vcvtq_s32_f32(fx));

      /* if greater, substract 1 */
      fx = tmp - vcvtq_n_f32_u32(vcgtq_f32(tmp, fx), 32);

      x = vmlsq_f32(x, fx, vdupq_n_f32(0.69357156944f));


      static const float c[6] = {
        5.0000001201E-1f,
        1.6666665459E-1f,
        4.1665795894E-2f,
        8.3334519073E-3f,
        1.3981999507E-3f,
        1.9875691500E-4f
      };

      auto x2 = x * x;
      auto x3 = x2 * x;
      auto x4 = x2 * x2;
      auto x5 = x2 * x3;

      auto y = vld1q_dup_f32(c + 0);
      y += vld1q_dup_f32(c + 1) * x;
      y += vld1q_dup_f32(c + 2) * x2;
      y += vld1q_dup_f32(c + 3) * x3;
      y += vld1q_dup_f32(c + 4) * x4;
      y += vld1q_dup_f32(c + 5) * x5;

      y = vmlaq_f32(x, y, x2);

      /* build 2^n */
      auto mm = vaddq_s32(vcvtq_s32_f32(fx), vdupq_n_s32(0x7f));
      auto pow2n = vreinterpretq_f32_s32(vshlq_n_s32(mm, 23));

      y = vmlaq_f32(pow2n, pow2n, y);

      return y;
#endif
    }

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

    inline float32x4_t pow_f32(const float32x4_t x, const float32x4_t m) {
      return exp_f32(m * simd_log(x));
    }

    inline float32x4_t sqrt(const float32x4_t x) {
      float _x[4];
      vst1q_f32(_x, x);
      for (int i = 0; i < 4; i++) {
        _x[i] = sqrtf(_x[i]);
      }
      return vld1q_f32(_x);
    }

    inline float32x4_t  cbrt(const float32x4_t x) {
      float _x[4];
      vst1q_f32(_x, x);
      for (int i = 0; i < 4; i++) {
        _x[i] = cbrtf(_x[i]);
      }
      return vld1q_f32(_x);
    }

    inline float32x4_t sqrt2(const float32x4_t x) {
      float _x[4];
      vst1q_f32(_x, x);
      for (int i = 0; i < 4; i++) {
        _x[i] = sqrtf(sqrtf(_x[i]));
      }
      return vld1q_f32(_x);
    }

    inline float32x4_t lnot(const float32x4_t x) {
      return vdupq_n_f32(1) - x;
    }

    inline float32x4_t clamp_n_low_base(const float32x4_t in, float min) {
      auto m = vdupq_n_f32(min);
      return vmaxq_f32(in + m, m);
    }

    inline float32x4_t clamp_n(const float32x4_t in, float min, float max) {
      auto _min = vdupq_n_f32(min);
      auto _max = vdupq_n_f32(max);
      return vminq_f32(_max, vmaxq_f32(_min, in));
    }

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
      auto x  = in;
      auto x2 = x * x;

      float32x4_t a;
      a = vmlaq_f32(vdupq_n_f32(17325), x2, x2 + vdupq_n_f32(378));
      a = vmlaq_f32(vdupq_n_f32(135135), x2, a) * x;

      float32x4_t b;
      b = vmlaq_f32(vdupq_n_f32(3150), x2, vdupq_n_f32(28));
      b = vmlaq_f32(vdupq_n_f32(62370), x2, b);
      b = vmlaq_f32(vdupq_n_f32(135135), x2, b);

      return a * invert(b);
    }

    inline float32x4_t atan(const float32x4_t in) {
      auto one = vdupq_n_f32(1);
      auto c1 = vdupq_n_f32(0.2447);
      auto c2 = vdupq_n_f32(0.0663);

      auto piOver4 = vdupq_n_f32(M_PI_4);
      auto xabs = vabdq_f32(in, vdupq_n_f32(0));
      return piOver4 * in - in * (xabs - one) * (c1 + c2 * xabs);
      //return M_PI_4*x - x*(fabs(x) - 1)*(0.2447 + 0.0663*fabs(x));
    }

    inline void sincos_f32(float32x4_t x, float32x4_t *ysin, float32x4_t *ycos) {
#if false
      simd_sincos(x, ysin, ycos);
#else

#define c_minus_cephes_DP1 -0.78515625
#define c_minus_cephes_DP2 -2.4187564849853515625e-4
#define c_minus_cephes_DP3 -3.77489497744594108e-8
#define c_sincof_p0 -1.9515295891E-4
#define c_sincof_p1 8.3321608736E-3
#define c_sincof_p2 -1.6666654611E-1
#define c_coscof_p0 2.443315711809948E-005
#define c_coscof_p1 -1.388731625493765E-003
#define c_coscof_p2 4.166664568298827E-002
#define c_cephes_FOPI 1.27323954473516 // 4 / M_PI

      float32x4_t xmm1, xmm2, xmm3, y;

      uint32x4_t emm2;

      uint32x4_t sign_mask_sin, sign_mask_cos;
      sign_mask_sin = vcltq_f32(x, vdupq_n_f32(0));
      x = vabsq_f32(x);

      /* scale by 4/Pi */
      y = vmulq_f32(x, vdupq_n_f32(c_cephes_FOPI));

      /* store the integer part of y in mm0 */
      emm2 = vcvtq_u32_f32(y);
      /* j=(j+1) & (~1) (see the cephes sources) */
      emm2 = vaddq_u32(emm2, vdupq_n_u32(1));
      emm2 = vandq_u32(emm2, vdupq_n_u32(~1));
      y = vcvtq_f32_u32(emm2);

      /* get the polynom selection mask
      there is one polynom for 0 <= x <= Pi/4
      and another one for Pi/4<x<=Pi/2

      Both branches will be computed.
      */
      uint32x4_t poly_mask = vtstq_u32(emm2, vdupq_n_u32(2));

      /* The magic pass: "Extended precision modular arithmetic"
      x = ((x - y * DP1) - y * DP2) - y * DP3; */
      xmm1 = vmulq_n_f32(y, c_minus_cephes_DP1);
      xmm2 = vmulq_n_f32(y, c_minus_cephes_DP2);
      xmm3 = vmulq_n_f32(y, c_minus_cephes_DP3);
      x = vaddq_f32(x, xmm1);
      x = vaddq_f32(x, xmm2);
      x = vaddq_f32(x, xmm3);

      sign_mask_sin = veorq_u32(sign_mask_sin, vtstq_u32(emm2, vdupq_n_u32(4)));
      sign_mask_cos = vtstq_u32(vsubq_u32(emm2, vdupq_n_u32(2)), vdupq_n_u32(4));

      /* Evaluate the first polynom  (0 <= x <= Pi/4) in y1,
      and the second polynom      (Pi/4 <= x <= 0) in y2 */
      float32x4_t z = vmulq_f32(x, x);
      float32x4_t y1, y2;

      y1 = vmulq_n_f32(z, c_coscof_p0);
      y2 = vmulq_n_f32(z, c_sincof_p0);
      y1 = vaddq_f32(y1, vdupq_n_f32(c_coscof_p1));
      y2 = vaddq_f32(y2, vdupq_n_f32(c_sincof_p1));
      y1 = vmulq_f32(y1, z);
      y2 = vmulq_f32(y2, z);
      y1 = vaddq_f32(y1, vdupq_n_f32(c_coscof_p2));
      y2 = vaddq_f32(y2, vdupq_n_f32(c_sincof_p2));
      y1 = vmulq_f32(y1, z);
      y2 = vmulq_f32(y2, z);
      y1 = vmulq_f32(y1, z);
      y2 = vmulq_f32(y2, x);
      y1 = vsubq_f32(y1, vmulq_f32(z, vdupq_n_f32(0.5f)));
      y2 = vaddq_f32(y2, x);
      y1 = vaddq_f32(y1, vdupq_n_f32(1));

      /* select the correct result from the two polynoms */
      float32x4_t ys = vbslq_f32(poly_mask, y1, y2);
      float32x4_t yc = vbslq_f32(poly_mask, y2, y1);
      *ysin = vbslq_f32(sign_mask_sin, vnegq_f32(ys), ys);
      *ycos = vbslq_f32(sign_mask_cos, yc, vnegq_f32(yc));
#endif
    }

    inline float32x4_t tan(const float32x4_t f) {
      float32x4_t sin, cos;
      sincos_f32(f, &sin, &cos);
      return sin * invert(cos);
    }

    inline float32x4_t vpo_scale(
      const float32x4_t vpo,
      const float32x4_t f0
    ) {
      const float32x4_t vpoLogMax = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
      return clamp_n(f0 * util::simd::exp_f32(vpo * vpoLogMax), 1, globalConfig.sampleRate / 4);
    }

    inline float32x4_t vpo_scale_no_clamp(
      const float32x4_t vpo,
      const float32x4_t f0
    ) {
      const float32x4_t vpoLogMax = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
      return f0 * util::simd::exp_f32(vpo * vpoLogMax);
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

    inline float32x4_t exp_n_scale(const float32x4_t x, float min, float max) {
      auto logMin = vdupq_n_f32(logf(min));
      auto logMax = vdupq_n_f32(logf(max));
      return exp_f32(lerp(logMin, logMax, x));
    }

    inline float32x2_t make_f32(float a, float b) {
      float x[2];
      x[0] = a;
      x[1] = b;
      return vld1_f32(x);
    }

    inline float32x4_t makeq_f32(float a, float b, float c, float d) {
      float32x4_t x = vdupq_n_f32(0);
      x = vsetq_lane_f32(a, x, 0);
      x = vsetq_lane_f32(b, x, 1);
      x = vsetq_lane_f32(c, x, 2);
      x = vsetq_lane_f32(d, x, 3);
      return x;
    }

    inline float32x2_t padd_self(const float32x2_t v) {
      return vpadd_f32(v, v);
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

    inline float32x4_t pushq_f32(const float32x4_t v, const float n) {
      return vsetq_lane_f32(n, vextq_f32(v, v, 1), 3);
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
    inline void rtocq_f32_ptr(
      float *ptr,
      const float32x4_t a,
      const float32x4_t b,
      const float32x4_t c,
      const float32x4_t d
    ) {
      float32x4x2_t ab = vzipq_f32(a, b);
      float32x4x2_t cd = vzipq_f32(c, d);

      vst1q_f32(ptr + 0,  vcombine_f32( vget_low_f32(ab.val[0]),  vget_low_f32(cd.val[0])));
      vst1q_f32(ptr + 4,  vcombine_f32(vget_high_f32(ab.val[0]), vget_high_f32(cd.val[0])));
      vst1q_f32(ptr + 8,  vcombine_f32( vget_low_f32(ab.val[1]),  vget_low_f32(cd.val[1])));
      vst1q_f32(ptr + 12, vcombine_f32(vget_high_f32(ab.val[1]), vget_high_f32(cd.val[1])));
    }

    inline void print_f32(const float32x4_t x) {
      static int count = 0;
      count++;
      if (count > 2000) return;
      
      logRaw("%f %f %f %f\n",
        vgetq_lane_f32(x, 0),
        vgetq_lane_f32(x, 1),
        vgetq_lane_f32(x, 2),
        vgetq_lane_f32(x, 3)
      );
    }

    inline void print(const float32x2_t x) {
      static int count = 0;
      count++;
      if (count > 500) return;
      
      logRaw("%f %f\n",
        vget_lane_f32(x, 0),
        vget_lane_f32(x, 1)
      );
    }
  }

  struct Latch {
    inline uint32_t read(const uint32_t high) {
      mTrigger = high & mEnable;
      mEnable = ~high;
      return mTrigger;
    }

    uint32_t mEnable  = 0;
    uint32_t mTrigger = 0;
  };
}