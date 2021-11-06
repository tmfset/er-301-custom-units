#pragma once

#include <od/audio/Sample.h>
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
    #define c_inv_mant_mask ~0x7f800000u
    #define c_cephes_SQRTHF 0.707106781186547524
    #define c_cephes_log_p0 7.0376836292E-2
    #define c_cephes_log_p1 -1.1514610310E-1
    #define c_cephes_log_p2 1.1676998740E-1
    #define c_cephes_log_p3 -1.2420140846E-1
    #define c_cephes_log_p4 +1.4249322787E-1
    #define c_cephes_log_p5 -1.6668057665E-1
    #define c_cephes_log_p6 +2.0000714765E-1
    #define c_cephes_log_p7 -2.4999993993E-1
    #define c_cephes_log_p8 +3.3333331174E-1
    #define c_cephes_log_q1 -2.12194440e-4
    #define c_cephes_log_q2 0.693359375

    /* natural logarithm computed for 4 simultaneous float
    return NaN for x <= 0
    */
    inline float32x4_t log_f32(float32x4_t x) {
      float32x4_t one = vdupq_n_f32(1);

      x = vmaxq_f32(x, vdupq_n_f32(0)); /* force flush to zero on denormal values */
      uint32x4_t invalid_mask = vcleq_f32(x, vdupq_n_f32(0));

      int32x4_t ux = vreinterpretq_s32_f32(x);

      int32x4_t emm0 = vshrq_n_s32(ux, 23);

      /* keep only the fractional part */
      ux = vandq_s32(ux, vdupq_n_s32(c_inv_mant_mask));
      ux = vorrq_s32(ux, vreinterpretq_s32_f32(vdupq_n_f32(0.5f)));
      x = vreinterpretq_f32_s32(ux);

      emm0 = vsubq_s32(emm0, vdupq_n_s32(0x7f));
      float32x4_t e = vcvtq_f32_s32(emm0);

      e = vaddq_f32(e, one);

      /* part2:
      if( x < SQRTHF ) {
      e -= 1;
      x = x + x - 1.0;
      } else { x = x - 1.0; }
      */
      uint32x4_t mask = vcltq_f32(x, vdupq_n_f32(c_cephes_SQRTHF));
      float32x4_t tmp = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(x), mask));
      x = vsubq_f32(x, one);
      e = vsubq_f32(e,
                    vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(one), mask)));
      x = vaddq_f32(x, tmp);

      float32x4_t z = vmulq_f32(x, x);

      float32x4_t y = vdupq_n_f32(c_cephes_log_p0);
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p1));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p2));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p3));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p4));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p5));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p6));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p7));
      y = vmulq_f32(y, x);
      y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p8));
      y = vmulq_f32(y, x);

      y = vmulq_f32(y, z);

      tmp = vmulq_f32(e, vdupq_n_f32(c_cephes_log_q1));
      y = vaddq_f32(y, tmp);

      tmp = vmulq_f32(z, vdupq_n_f32(0.5f));
      y = vsubq_f32(y, tmp);

      tmp = vmulq_f32(e, vdupq_n_f32(c_cephes_log_q2));
      x = vaddq_f32(x, y);
      x = vaddq_f32(x, tmp);
      x = vreinterpretq_f32_u32(
          vorrq_u32(vreinterpretq_u32_f32(x), invalid_mask)); // negative arg will be NAN
      return x;
    }

    inline float32x4_t exp_f32(float32x4_t x) {
#ifdef USE_SSE4
    return simd_exp(x);
#else

    #define c_exp_hi 88.3762626647949f
    #define c_exp_lo -88.3762626647949f

    #define c_cephes_LOG2EF 1.44269504088896341
    #define c_cephes_exp_C1 0.693359375
    #define c_cephes_exp_C2 -2.12194440e-4

    #define c_cephes_exp_p0 1.9875691500E-4
    #define c_cephes_exp_p1 1.3981999507E-3
    #define c_cephes_exp_p2 8.3334519073E-3
    #define c_cephes_exp_p3 4.1665795894E-2
    #define c_cephes_exp_p4 1.6666665459E-1
    #define c_cephes_exp_p5 5.0000001201E-1

    float32x4_t tmp, fx;

    float32x4_t one = vdupq_n_f32(1);
    x = vminq_f32(x, vdupq_n_f32(c_exp_hi));
    x = vmaxq_f32(x, vdupq_n_f32(c_exp_lo));

    /* express exp(x) as exp(g + n*log(2)) */
    fx = vmlaq_f32(vdupq_n_f32(0.5f), x, vdupq_n_f32(c_cephes_LOG2EF));

    /* perform a floorf */
    tmp = vcvtq_f32_s32(vcvtq_s32_f32(fx));

    /* if greater, substract 1 */
    uint32x4_t mask = vcgtq_f32(tmp, fx);
    mask = vandq_u32(mask, vreinterpretq_u32_f32(one));

    fx = vsubq_f32(tmp, vreinterpretq_f32_u32(mask));

    tmp = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C1));
    float32x4_t z = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C2));
    x = vsubq_f32(x, tmp);
    x = vsubq_f32(x, z);

    float32x4_t y = vdupq_n_f32(c_cephes_exp_p0);
    float32x4_t c1 = vdupq_n_f32(c_cephes_exp_p1);
    float32x4_t c2 = vdupq_n_f32(c_cephes_exp_p2);
    float32x4_t c3 = vdupq_n_f32(c_cephes_exp_p3);
    float32x4_t c4 = vdupq_n_f32(c_cephes_exp_p4);
    float32x4_t c5 = vdupq_n_f32(c_cephes_exp_p5);

    y = vmulq_f32(y, x);
    z = vmulq_f32(x, x);
    y = vaddq_f32(y, c1);
    y = vmulq_f32(y, x);
    y = vaddq_f32(y, c2);
    y = vmulq_f32(y, x);
    y = vaddq_f32(y, c3);
    y = vmulq_f32(y, x);
    y = vaddq_f32(y, c4);
    y = vmulq_f32(y, x);
    y = vaddq_f32(y, c5);

    y = vmulq_f32(y, z); // here
    y = vaddq_f32(y, x);
    y = vaddq_f32(y, one);

    /* build 2^n */
    int32x4_t mm;
    mm = vcvtq_s32_f32(fx);
    mm = vaddq_s32(mm, vdupq_n_s32(0x7f));
    mm = vshlq_n_s32(mm, 23);

    float32x4_t pow2n = vreinterpretq_f32_s32(mm);

    y = vmulq_f32(y, pow2n);

    return y;
#endif
    }

    inline float32x4_t pow_f32(float32x4_t x, float32x4_t m) {
      return exp_f32(m * log_f32(x));
    }

    inline float32x4_t pow_unit_f32(float32x4_t x, float32x4_t m) {
      auto zero = vdupq_n_f32(0);
      auto one = vdupq_n_f32(1);
      auto p = exp_f32(m * log_f32(x));
      return vbslq_f32(
        vcleq_f32(x, zero),
        zero,
        vmaxq_f32(zero, vminq_f32(one, p))
      );
    }

    inline float32x4_t toDecibels(float32x4_t x) {
      // 20 * ln(x) / ln(10)
      x = vmaxq_f32(x, vdupq_n_f32(0.001));
      return vdupq_n_f32(20) * log_f32(x) * vdupq_n_f32(0.4342944819f);
    }

    inline float32x4_t fromDecibels(float32x4_t x) {
      // 10^(x/20)
      return pow_f32(vdupq_n_f32(10), x * vdupq_n_f32(0.05));
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

    inline float32x4_t sqrt(const float32x4_t x) {
      float _x[4];
      vst1q_f32(_x, x);
      for (int i = 0; i < 4; i++) {
        _x[i] = sqrtf(_x[i]);
      }
      return vld1q_f32(_x);
    }

    inline float32x4_t cbrt(const float32x4_t x) {
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

    // Wrap [0, 1]
    inline float32x4_t wrap(float32x4_t x) {
      x = x - floor(x);
      x = x + vdupq_n_f32(1);
      x = x - floor(x);
      return x;
    }

    inline float32x4x2_t mix(float32x4_t by) {
      auto half = vdupq_n_f32(0.5);
      auto one  = vdupq_n_f32(1);

      auto rightAmount = by * half + half;
      auto leftAmount  = one - rightAmount;

      return {{ leftAmount, rightAmount }};
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

    inline float32x4_t vpo_scalar(const float32x4_t vpo) {
      const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      const float32x4_t vpoLogMax = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
      return util::simd::exp_f32(vpo * vpoLogMax) * sp;
    }

    inline float32x4_t vpo_scale(
      const float32x4_t vpo,
      const float32x4_t f0
    ) {
      const float32x4_t vpoLogMax = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
      return clamp_n(f0 * util::simd::exp_f32(vpo * vpoLogMax), 0.001, globalConfig.sampleRate / 4);
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

    inline uint32x4_t vld1q_dup_cgtz(const float* p) {
      auto zero = vdupq_n_f32(0);
      return vcgtq_f32(vld1q_dup_f32(p), zero);
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
      float _x[4];
      _x[0] = a;
      _x[1] = b;
      _x[2] = c;
      _x[3] = d;
      return vld1q_f32(_x);
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

    inline void print_f(float x) {
      logRaw("%f\n", x);
    }

    inline void print_u32(const uint32x4_t x, bool newline = false) {
      static int count = 0;
      count++;
      if (count > 4000) return;
      
      logRaw("%08x %08x %08x %08x\n",
        vgetq_lane_u32(x, 0),
        vgetq_lane_u32(x, 1),
        vgetq_lane_u32(x, 2),
        vgetq_lane_u32(x, 3)
      );

      if (newline) {
        logRaw("\n");
      }
    }

    inline void print_f32(const float32x4_t x, bool newline = false) {
      static int count = 0;
      count++;
      if (count > 4000) return;
      
      logRaw("%f %f %f %f\n",
        vgetq_lane_f32(x, 0),
        vgetq_lane_f32(x, 1),
        vgetq_lane_f32(x, 2),
        vgetq_lane_f32(x, 3)
      );

      if (newline) {
        logRaw("\n");
      }
    }

    inline void print_newline() {
      logRaw("\n");
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

    inline float32x4_t vtnh(uint32_t *track, float base, float32x4_t other) {
      float _out[4], _other[4];
      vst1q_f32(_other, other);
      for (int i = 0; i < 4; i++) {
        if (track[i]) { base = _other[i]; }
        _out[i] = base;
      }
      return vld1q_f32(_out);
    }

    inline float32x4_t readSample(od::Sample *sample, const int *index, int channel) {
      float32x4_t out = vdupq_n_f32(0);
      out = vsetq_lane_f32(sample->get(index[0], channel), out, 0);
      out = vsetq_lane_f32(sample->get(index[1], channel), out, 1);
      out = vsetq_lane_f32(sample->get(index[2], channel), out, 2);
      out = vsetq_lane_f32(sample->get(index[3], channel), out, 3);
      return out;
    }

    inline void writeSample(od::Sample *sample, const int *index, int channel, float32x4_t value) {
      sample->set(index[0], channel, vgetq_lane_f32(value, 0));
      sample->set(index[1], channel, vgetq_lane_f32(value, 1));
      sample->set(index[2], channel, vgetq_lane_f32(value, 2));
      sample->set(index[3], channel, vgetq_lane_f32(value, 3));
    }

    struct Complement {
      float32x4_t mValue;
      float32x4_t mComplement;

      inline Complement() {}

      inline Complement(float32x4_t value, float32x4_t one) {
        mValue      = value;
        mComplement = one - value;
      }

      inline float32x4_t lerp(const float32x4_t &from, const float32x4_t &to) const {
        return vmlaq_f32(from * mValue, to, mComplement);
      }

      inline float32x4_t ease(const float32x4_t &from) const {
        return vmlaq_f32(mComplement, from, mValue);
      }
    };
  }

  inline int    max(  int a,   int b) { return a > b ? a : b; }
  inline float fmax(float a, float b) { return a > b ? a : b; }
  inline int    min(  int a,   int b) { return a < b ? a : b; }
  inline float fmin(float a, float b) { return a < b ? a : b; }

  inline int    clamp(  int v,  int _min,  int _max) { return min(max(v, _min), _max); }
  inline float fclamp(float v, float min, float max) { return fmin(fmax(v, min), max); }

  inline float  funit(float v) { return fclamp(v, -1, 1); }
  inline float fpunit(float v) { return fclamp(v, 0, 1); }

  inline int mod(int a, int n) { return ((a % n) + n) % n; }

  inline int moddst(int a, int b, int n) {
    int m = mod(a - b, n);
    return min(m, n - m);
  }

  inline int fhr(float v) {
    int iv = v;
    float d = v - iv;
    return d >= 0.5f ? iv + 1 : iv;
  }

  inline float toDecibels(float x) {
    // 20 * ln(x) / ln(10)
    x = fmax(x, 0.001);
    return 20.0f * logf(x) * 0.4342944819f;
  }

  inline float fromDecibels(float x) {
    // 10^(x/20)
    return powf(10.0f, x * 0.05);
  }

  inline uint32_t bcvt(const bool b) {
    return b ? 0xffffffff : 0;
  }

  inline uint32_t fcgt(const float x, const float threshold) {
    return bcvt(x > threshold);
  }

  struct Trigger {
    inline uint32_t read(const uint32_t high) {
      mTrigger = high & mEnable;
      mEnable = ~high;
      return mTrigger;
    }

    uint32_t mEnable  = 0;
    uint32_t mTrigger = 0;
  };

  struct Latch {
    inline uint32_t read(
      const uint32_t set,
      const uint32_t reset
    ) {
      mState = mState & ~reset;
      mState = mState | set;
      return mState;
    }

    uint32_t mState = 0;
  };

  namespace two {
    // Lerp imprecise. Does not guarantee output = to when by = 1
    inline float32x2_t lerpi(float32x2_t from, float32x2_t to, float32x2_t by) {
      return vadd_f32(from, vmul_f32(by, vsub_f32(to, from)));
    }

    // Lerp precise. Guarantees output = to when by = 1
    inline float32x2_t lerpp(float32x2_t from, float32x2_t to, float32x2_t by) {
      return vadd_f32(
        vmul_f32(vsub_f32(vdup_n_f32(1), by), from),
        vmul_f32(by, to)
      );
    }

    inline float32x2_t exp_f32(float32x2_t x) {
      return vget_low_f32(simd::exp_f32(vcombine_f32(x, x)));
    }

    inline float32x2_t exp_ns_f32(float32x2_t x, float min, float max) {
      auto logMin = vdup_n_f32(logf(min));
      auto logMax = vdup_n_f32(logf(max));
      return exp_f32(lerpi(logMin, logMax, x));
    }

    inline float32x2_t tan(float32x2_t x) {
      return vget_low_f32(simd::tan(vcombine_f32(x, x)));
    }

    inline float32x2_t fclamp(float32x2_t in, float32x2_t min, float32x2_t max) {
      return vmin_f32(max, vmax_f32(min, in));
    }

    inline float32x2_t fclamp_n(float32x2_t in, float min, float max) {
      return fclamp(in, vdup_n_f32(min), vdup_n_f32(max));
    }

    inline float32x2_t vpo_scale(float32x2_t f0, float32x2_t vpo) {
      return f0 * exp_f32(vpo * vdup_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f)));
    }

    inline float32x2_t vpo_scale_limited(float32x2_t f0, float32x2_t vpo) {
      return fclamp_n(vpo_scale(f0, vpo), 0.001, globalConfig.sampleRate / 4);
    }

    // https://en.wikipedia.org/wiki/Division_algorithm#Newton.E2.80.93Raphson_division
    // iterate 3 times for 24 bits of precision
    inline float32x2_t invert(float32x2_t x) {
      float32x2_t inv;
      inv = vrecpe_f32(x);
      inv = vmul_f32(inv, vrecps_f32(x, inv));
      inv = vmul_f32(inv, vrecps_f32(x, inv));
      inv = vmul_f32(inv, vrecps_f32(x, inv));
      return inv;
    }
  }

  namespace four {
    struct TrackAndHold {
      inline TrackAndHold(float initial) {
        mValue = vdupq_n_f32(initial);
      }

      inline void set(const float32x4_t signal) {
        mValue = signal;
      }

      inline void track(const uint32x4_t gate, const float32x4_t signal) {
        mValue = vbslq_f32(gate, signal, mValue);
      }

      inline void track(const uint32x4_t gate, const TrackAndHold &other) {
        track(gate, other.value());
      }

      inline float32x4_t value() const { return mValue; }

      float32x4_t mValue = vdupq_n_f32(0);
    };

    struct Latch {
      inline uint32x4_t read(
        const uint32x4_t set,
        const uint32x4_t reset
      ) {
        mState = vandq_u32(mState, vmvnq_u32(reset));
        mState = vorrq_u32(mState, set);
        return mState;
      }

      uint32x4_t mState = vdupq_n_u32(0);
    };

    struct Trigger {
      inline uint32x4_t read(const uint32x4_t high) {
        mTrigger = vandq_u32(high, mEnable);
        mEnable = vmvnq_u32(high);
        return mTrigger;
      }

      uint32x4_t mEnable = vdupq_n_u32(0);
      uint32x4_t mTrigger = vdupq_n_u32(0);
    };

    struct Vpo {
      inline void track(const uint32x4_t gate, const Vpo &other) {
        mScale.track(gate, other.mScale);
      }

      inline float32x4_t freq(const float32x4_t f0) const {
        return f0 * mScale.value();
      }

      inline float32x4_t freqEnv(const float32x4_t f0, const float32x4_t env) const {
        return f0 * util::simd::lerp(vdupq_n_f32(1), mScale.value(), env);
      }

      inline float32x4_t delta(const float32x4_t f0) const {
        const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
        return freq(f0) * sp;
      }

      inline float32x4_t deltaOffset(const float32x4_t f0, const Vpo& offset) const {
        return delta(offset.freq(f0));
      }

      inline void configure(const float32x4_t vpo) {
        const float32x4_t vpoLogMax = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
        mScale.set(simd::exp_f32(vpo * vpoLogMax));
      }

      TrackAndHold mScale { 1 };
    };
  }
}