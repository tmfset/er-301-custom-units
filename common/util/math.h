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

#define CENTS_PER_OCTAVE 1200

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

    // https://stackoverflow.com/questions/47025373/fastest-implementation-of-the-natural-exponential-function-using-sse
    // Quite fast, but not *super* accurate.
    // max. rel. error = 3.55959567e-2 on [-87.33654, 88.72283]
    // Which doesn't matter much since we're normally in [-1, 1]
    inline float32x4_t fast_exp_f32(float32x4_t x) {
      float32x4_t a = vdupq_n_f32(12102203.0f);
      int32x4_t b = vdupq_n_s32(127 * (1 << 23) - 298765);
      int32x4_t t = b + vcvtq_s32_f32(a * x);
      return vreinterpretq_f32_s32(t);
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

    inline float32x4_t sqrt(const float32x4_t x) {
      float _x[4];
      vst1q_f32(_x, x);
      for (int i = 0; i < 4; i++) {
        _x[i] = sqrtf(_x[i]);
      }
      return vld1q_f32(_x);
    }

    inline float32x4_t sqrt_lim(float32x4_t x) {
      auto sign = vcltq_f32(x, vdupq_n_f32(0));
      x = sqrt(vabsq_f32(x));
      return vbslq_f32(sign, vnegq_f32(x), x);
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

    #define LOG2 0.6931471805599453f
    #define VPO_LOG_MAX FULLSCALE_IN_VOLTS * LOG2

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

    inline void print_f(float x, bool newline = false) {
      logRaw("%f\n", x);
      if (newline) logRaw("\n");
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

  inline float fabs(float v) { return v >= 0 ? v : -v; }

  inline int    max(  int a,   int b) { return a > b ? a : b; }
  inline float fmax(float a, float b) { return a > b ? a : b; }
  inline int    min(  int a,   int b) { return a < b ? a : b; }
  inline float fmin(float a, float b) { return a < b ? a : b; }
  inline float fcenter(float low, float high) { return high - ((high - low) / 2.0f); }

  inline int    clamp(  int v,  int _min,  int _max) { return min(max(v, _min), _max); }
  inline float fclamp(float v, float min, float max) { return fmin(fmax(v, min), max); }

  // Lerp imprecise. Does not guarantee output = to when by = 1
  inline float flerpi(float from, float to, float by) {
    return from + by * (to - from);
  }

  inline float fpclamp(float v, float a, float b) {
    return fclamp(v, fmin(a, b), fmax(a, b));
  }

  inline float  funit(float v) { return fclamp(v, -1, 1); }
  inline float fpunit(float v) { return fclamp(v, 0, 1); }

  inline int mod(int a, int n) { return ((a % n) + n) % n; }

  inline int moddst(int a, int b, int n) {
    int m = mod(a - b, n);
    return min(m, n - m);
  }

  inline float fmod(float a, int n) {
    auto i = (int)a;
    auto diff = a - i;
    return mod(i, n) + diff;
  }

  // inline float fmoddst(float a, float b, int n) {
  //   auto m = fmod(a - b, n);
  //   return fmin(m, n - m);
  // }

  // Float down-round
  inline int fdr(float v) {
    int iv = v;
    return v < 0 ? iv - 1 : iv;
  }

  // Float half-round
  inline int fhr(float v) {
    return fdr(v + 0.5);
  }

  // Float up-round
  inline int fur(float v) {
    return fdr(v + 1);
  }

  inline int ceil(float v) {
    int iv = (int)v;
    if (v < 0) return iv;
    if (v > iv) return iv + 1;
    return iv;
  }

  // Float round to precision
  inline float frt(float v, float rounding) {
    return fhr(v / rounding) * rounding;
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

  inline float toVoltage(float x)   { return x * FULLSCALE_IN_VOLTS; }
  inline float fromVoltage(float x) { return x / FULLSCALE_IN_VOLTS; }
  inline float toCents(float x)     { return x * CENTS_PER_OCTAVE; }
  inline float fromCents(float x)   { return x / CENTS_PER_OCTAVE; }

  inline float toOctave(float x) {
    return (int)(x + FULLSCALE_IN_VOLTS) - FULLSCALE_IN_VOLTS;
  }

  inline float toPercent(float input)   { return input * 100.0f; }
  inline float fromPercent(float input) { return input * 0.01f; }

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

  namespace four {
    inline float32x4_t make(float a, float b) {
      return vcombine_f32(vdup_n_f32(a), vdup_n_f32(b));
    }

    inline float32x4_t make(float a, float b, float c, float d) {
      float _x[4];
      _x[0] = a;
      _x[1] = b;
      _x[2] = c;
      _x[3] = d;
      return vld1q_f32(_x);
    }

    inline uint32x4_t make_u32(bool a, bool b, bool c, bool d) {
      uint32_t _x[4];
      _x[0] = util::bcvt(a);
      _x[1] = util::bcvt(b);
      _x[2] = util::bcvt(c);
      _x[3] = util::bcvt(d);
      return vld1q_u32(_x);
    }

    inline float32x4_t comp(float32x4_t x) {
      return vdupq_n_f32(1) - x;
    }

    inline float32x4_t twice(const float32x4_t x) {
      return x + x;
    }

    // Add all lanes together.
    inline float sum_lanes(const float32x4_t v) {
      auto pair = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
      pair = vpadd_f32(pair, pair);
      return vget_lane_f32(pair, 0);
    }

    // Reverse the lane order.
    inline float32x4_t reverse(const float32x4_t v) {
      auto pair = vrev64q_f32(v);
      return vcombine_f32(vget_high_f32(pair), vget_low_f32(pair));
    }

    // Push a new value into the last lane.
    inline float32x4_t push(const float32x4_t v, const float n) {
      return vsetq_lane_f32(n, vextq_f32(v, v, 1), 3);
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

    // Lerp imprecise. Does not guarantee output = to when by = 1
    inline float32x4_t lerpi(float32x4_t from, float32x4_t to, float32x4_t by) {
      return from + by * (to - from);
    }

    // Lerp precise. Guarantees output = to when by = 1
    inline float32x4_t lerpp(float32x4_t from, float32x4_t to, float32x4_t by) {
      return comp(by) * from + by * to;
    }

    inline float32x4_t exp_ns_f32(float32x4_t x, const float min, const float max) {
      auto logMin = vdupq_n_f32(logf(min));
      auto logMax = vdupq_n_f32(logf(max));
      return simd::exp_f32(lerpi(logMin, logMax, x));
    }

    inline float32x4_t fast_exp_ns_f32(float32x4_t x, const float min, const float max) {
      auto logMin = vdupq_n_f32(logf(min));
      auto logMax = vdupq_n_f32(logf(max));
      return simd::fast_exp_f32(lerpi(logMin, logMax, x));
    }

    inline float32x4_t fclamp(float32x4_t x, float32x4_t min, float32x4_t max) {
      return vminq_f32(max, vmaxq_f32(min, x));
    }

    inline float32x4_t fclamp_n(float32x4_t x, float min, float max) {
      return fclamp(x, vdupq_n_f32(min), vdupq_n_f32(max));
    }

    inline float32x4_t fclamp_unit(float32x4_t x) {
      return fclamp_n(x, -1, 1);
    }

    inline float32x4_t fclamp_punit(float32x4_t x) {
      return fclamp_n(x, 0, 1);
    }

    inline float32x4_t fclamp_freq(float32x4_t x) {
      return fclamp_n(x, 0.001, globalConfig.sampleRate / 4);
    }

    inline float32x4_t fscale_vpo(float32x4_t vpo) {
      return vpo * vdupq_n_f32(VPO_LOG_MAX);
    }

    inline float32x4_t vpo_scale(float32x4_t f0, float32x4_t vpo) {
      return f0 * simd::exp_f32(fscale_vpo(vpo));
    }

    inline float32x4_t vpo_scale_limited(float32x4_t f0, float32x4_t vpo) {
      return fclamp_freq(vpo_scale(f0, vpo));
    }

    inline float32x4_t fast_vpo_scale(float32x4_t f0, float32x4_t vpo) {
      return f0 * simd::fast_exp_f32(fscale_vpo(vpo));
    }

    inline float32x4_t fast_vpo_scale_limited(float32x4_t f0, float32x4_t vpo) {
      return fclamp_freq(fast_vpo_scale(f0, vpo));
    }

    // https://en.wikipedia.org/wiki/Division_algorithm#Newton.E2.80.93Raphson_division
    // iterate 3 times for 24 bits of precision
    inline float32x4_t invert(const float32x4_t x) {
      float32x4_t inv;
      inv = vrecpeq_f32(x);
      inv *= vrecpsq_f32(x, inv);
      inv *= vrecpsq_f32(x, inv);
      inv *= vrecpsq_f32(x, inv);
      return inv;
    }

    inline float32x4_t tan(const float32x4_t f) {
      float32x4_t sin, cos;
      simd::sincos_f32(f, &sin, &cos);
      return sin * invert(cos);
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

      return a * four::invert(b);
    }

    inline float32x4_t atan(const float32x4_t in) {
      auto one = vdupq_n_f32(1);
      auto c1 = vdupq_n_f32(0.2447);
      auto c2 = vdupq_n_f32(0.0663);

      auto piOver4 = vdupq_n_f32(M_PI_4);
      auto xabs = vabdq_f32(in, vdupq_n_f32(0));
      return piOver4 * in - in * (xabs - one) * (c1 + c2 * xabs);
    }

    inline float32x4x2_t mix(float32x4_t by) {
      auto half = vdupq_n_f32(0.5);
      auto one  = vdupq_n_f32(1);

      auto rightAmount = by * half + half;
      auto leftAmount  = one - rightAmount;

      return {{ leftAmount, rightAmount }};
    }

    struct ThreeWay {
      inline static ThreeWay punit(float32x4_t mix) {
        mix = fclamp_punit(mix);
        auto center = vdupq_n_f32(0.5f);
        auto select = vcltq_f32(mix, center);
        return ThreeWay { select, twice(vabdq_f32(mix, center)) };
      }

      inline ThreeWay(uint32_t select, float degree) :
        mSelect(vdupq_n_u32(select)),
        mDegree(vdupq_n_f32(degree)) { }

      inline ThreeWay(uint32x4_t select, float32x4_t degree) :
        mSelect(select),
        mDegree(degree) { }

      inline float32x4_t mix(float32x4_t bottom, float32x4_t middle, float32x4_t top) const {
        auto out = vbslq_f32(mSelect, bottom, top);
        return lerpi(middle, out, mDegree);
      }

      const uint32x4_t mSelect;
      const float32x4_t mDegree;
    };

    struct ThreeWayMix {
      inline ThreeWayMix(float min, float max, float center) :
        mMin(vdupq_n_f32(min)),
        mMax(vdupq_n_f32(max)),
        mCenter(vdupq_n_f32(center)),
        mScaleLow(vdupq_n_f32(1.0f / fabs(center - min))),
        mScaleHigh(vdupq_n_f32(1.0f / fabs(center - max))) { }

      inline ThreeWay prepare(float32x4_t mix) const {
        mix = fclamp(mix, mMin, mMax);
        auto select = vcltq_f32(mix, mCenter);
        auto scale = vbslq_f32(select, mScaleLow, mScaleHigh);
        return ThreeWay { select, scale * vabdq_f32(mix, mCenter) };
      }

      const float32x4_t mMin;
      const float32x4_t mMax;
      const float32x4_t mCenter;
      const float32x4_t mScaleLow;
      const float32x4_t mScaleHigh;
    };
  }

  namespace two {
    inline float32x2_t make(float a, float b) {
      float x[2];
      x[0] = a;
      x[1] = b;
      return vld1_f32(x);
    }

    inline float32x4_t dual(float32x2_t x) {
      return vcombine_f32(x, x);
    }

    inline float32x2_t padd(float32x4_t x) {
      return vadd_f32(vget_low_f32(x), vget_high_f32(x));
    }

    inline float32x2_t padds(float32x2_t x) {
      return vpadd_f32(x, x);
    }

    inline float32x2_t comp(float32x2_t x) {
      return vsub_f32(vdup_n_f32(1), x);
    }

    // Lerp imprecise. Does not guarantee output = to when by = 1
    inline float32x2_t lerpi(float32x2_t from, float32x2_t to, float32x2_t by) {
      return vadd_f32(from, vmul_f32(by, vsub_f32(to, from)));
    }

    // Lerp precise. Guarantees output = to when by = 1
    inline float32x2_t lerpp(float32x2_t from, float32x2_t to, float32x2_t by) {
      return vadd_f32(vmul_f32(comp(by), from), vmul_f32(by, to));
    }

    inline float32x2_t exp_f32(float32x2_t x) {
      return vget_low_f32(simd::exp_f32(dual(x)));
    }

    inline float32x2_t exp_ns_f32(float32x2_t x, const float min, const float max) {
      auto logMin = vdup_n_f32(logf(min));
      auto logMax = vdup_n_f32(logf(max));
      return exp_f32(lerpi(logMin, logMax, x));
    }

    inline float32x2_t tanh(float32x2_t x) {
      return vget_low_f32(four::tanh(dual(x)));
    }

    inline float32x2_t tan(float32x2_t x) {
      return vget_low_f32(four::tan(dual(x)));
    }

    inline float32x2_t sqrt(float32x2_t x) {
      return make(
        sqrtf(vget_lane_f32(x, 0)),
        sqrtf(vget_lane_f32(x, 1))
      );
    }

    inline float32x2_t sqrt_lim(float32x2_t x) {
      auto sign = vclt_f32(x, vdup_n_f32(0));
      x = sqrt(vabs_f32(x));
      return vbsl_f32(sign, vneg_f32(x), x);
    }

    inline float32x2_t twice(float32x2_t x) {
      return vadd_f32(x, x);
    }

    inline float32x2_t half(float32x2_t x) {
      return vmul_f32(x, vdup_n_f32(0.5));
    }

    inline float32x2_t add3(float32x2_t a, float32x2_t b, float32x2_t c) {
      return vadd_f32(a, vadd_f32(b, c));
    }

    inline float32x2_t favg(float32x2_t x, float32x2_t y) {
      return half(vadd_f32(x, y));
    }

    inline float32x2_t fcenter(float32x2_t low, float32x2_t high) {
      return vsub_f32(high, half(vsub_f32(high, low)));
    }

    inline float32x2_t fclamp(float32x2_t x, float32x2_t min, float32x2_t max) {
      return vmin_f32(max, vmax_f32(min, x));
    }

    inline float32x2_t fclamp_n(float32x2_t x, float min, float max) {
      return fclamp(x, vdup_n_f32(min), vdup_n_f32(max));
    }

    inline float32x2_t fclamp_unit(float32x2_t x) {
      return fclamp_n(x, -1, 1);
    }

    inline float32x2_t fclamp_punit(float32x2_t x) {
      return fclamp_n(x, 0, 1);
    }

    inline float32x2_t fclamp_freq(float32x2_t x) {
      return fclamp_n(x, 0.001, globalConfig.sampleRate / 4);
    }

    inline float32x2_t fscale_vpo(float32x2_t vpo) {
      return vmul_f32(vpo, vdup_n_f32(VPO_LOG_MAX));
    }

    inline float32x2_t vpo_scale(float32x2_t f0, float32x2_t vpo) {
      return vmul_f32(f0, exp_f32(vmul_f32(vpo, vdup_n_f32(VPO_LOG_MAX))));
    }

    inline float32x2_t vpo_scale_limited(float32x2_t f0, float32x2_t vpo) {
      return fclamp_freq(vpo_scale(f0, vpo));
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

    struct ThreeWay {
      inline static ThreeWay punit(float32x2_t mix) {
        mix = fclamp_punit(mix);
        auto center = vdup_n_f32(0.5f);
        auto select = vclt_f32(mix, center);
        return ThreeWay { select, twice(vabd_f32(mix, center)) };
      }

      inline ThreeWay(uint32_t select, float degree) :
        mSelect(vdup_n_u32(select)),
        mDegree(vdup_n_f32(degree)) { }

      inline ThreeWay(uint32x2_t select, float32x2_t degree) :
        mSelect(select),
        mDegree(degree) { }

      inline float32x2_t mix(float32x2_t bottom, float32x2_t middle, float32x2_t top) const {
        auto out = vbsl_f32(mSelect, bottom, top);
        return lerpi(middle, out, mDegree);
      }

      const uint32x2_t mSelect;
      const float32x2_t mDegree;
    };

    struct ThreeWayMix {
      inline ThreeWayMix(float min, float max, float center) :
        mMin(vdup_n_f32(min)),
        mMax(vdup_n_f32(max)),
        mCenter(vdup_n_f32(center)),
        mScaleLow(vdup_n_f32(1.0f / fabs(center - min))),
        mScaleHigh(vdup_n_f32(1.0f / fabs(center - max))) { }

      inline ThreeWay prepare(float32x2_t mix) const {
        auto select = vclt_f32(mix, mCenter);
        auto scale = vbsl_f32(select, mScaleLow, mScaleHigh);
        return ThreeWay { select, vmul_f32(scale, vabd_f32(mix, mCenter)) };
      }

      const float32x2_t mMin;
      const float32x2_t mMax;
      const float32x2_t mCenter;
      const float32x2_t mScaleLow;
      const float32x2_t mScaleHigh;
    };
  }

  namespace four {
    // Round towards zero
    inline float32x4_t frtz(float32x4_t v) {
      return vcvtq_f32_s32(vcvtq_s32_f32(v));
    }

    inline float32x4_t toVoltage(float32x4_t v)   { return v * vdupq_n_f32(FULLSCALE_IN_VOLTS); }
    inline float32x4_t fromVoltage(float32x4_t v) { return v * vdupq_n_f32(1.0f / FULLSCALE_IN_VOLTS); }
    inline float32x4_t toCents(float32x4_t v)     { return v * vdupq_n_f32(CENTS_PER_OCTAVE); }
    inline float32x4_t fromCents(float32x4_t v)   { return v * vdupq_n_f32(1.0f / CENTS_PER_OCTAVE); }

    inline float32x4_t toOctave(float32x4_t v) {
      auto fiv = vdupq_n_f32(FULLSCALE_IN_VOLTS);
      return frtz(v + fiv) - fiv;
    }

    inline bool anyFalse(uint32x4_t v) {
      auto t = vpmin_u32(vget_low_u32(v), vget_high_u32(v));
      return vget_lane_u32(vpmin_u32(t, t), 0) == 0;
    }

    inline bool anyTrue(uint32x4_t v) {
      auto t = vpmax_u32(vget_low_u32(v), vget_high_u32(v));
      return vget_lane_u32(vpmax_u32(t, t), 0) != 0;
    }

    inline uint32x4_t inRange(float32x4_t v, float32x4_t low, float32x4_t high) {
      return vandq_u32(vcgtq_f32(v, low), vcltq_f32(v, high));
    }

    inline uint32x4_t inRange(float32x4_t v, float low, float high) {
      return inRange(v, vdupq_n_f32(low), vdupq_n_f32(high));
    }

    inline uint32x4_t outOfRange(float32x4_t v, float32x4_t low, float32x4_t high) {
      return vorrq_u32(vcltq_f32(v, low), vcgtq_f32(v, high));
    }

    inline uint32x4_t outOfRange(float32x4_t v, float low, float high) {
      return outOfRange(v, vdupq_n_f32(low), vdupq_n_f32(high));
    }

    inline bool allInRange(float32x4_t v, float low, float high) {
      return !anyFalse(inRange(v, low, high));
    }

    inline bool anyOutOfRange(float32x4_t v, float low, float high) {
      return anyTrue(outOfRange(v, low, high));
    }

    inline float32x4_t select(int32x4_t i, float* fs) {
      return make(
        fs[vgetq_lane_s32(i, 0)],
        fs[vgetq_lane_s32(i, 1)],
        fs[vgetq_lane_s32(i, 2)],
        fs[vgetq_lane_s32(i, 3)]
      );
    }

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
        mState = mState & ~reset;
        mState = mState | set;
        return mState;
      }

      uint32x4_t mState = vdupq_n_u32(0);
    };

    struct Trigger {
      inline uint32x4_t read(const uint32x4_t high) {
        auto trigger = mEnable & high;
        mEnable = ~high;
        return trigger;
      }

      uint32x4_t mEnable = vdupq_n_u32(0);
    };

    struct SyncTrigger {
      inline uint32x4_t read(uint32x4_t high, uint32x4_t sync) {
        auto trigger = mTrigger.read(high);
        mLatch = mLatch | trigger; // set
        auto out = mLatch & sync;
        mLatch = mLatch & ~sync; // reset
        return out;
      }

      uint32x4_t mLatch = vdupq_n_u32(0);
      Trigger mTrigger;
    };

    struct Vpo {
      inline void track(const uint32x4_t gate, const Vpo &other) {
        mScale.track(gate, other.mScale);
      }

      inline float32x4_t freq(const float32x4_t f0) const {
        return f0 * mScale.value();
      }

      inline float32x4_t freqEnv(const float32x4_t f0, const float32x4_t env) const {
        return f0 * util::four::lerpi(vdupq_n_f32(1), mScale.value(), env);
      }

      inline float32x4_t delta(const float32x4_t f0) const {
        const auto sp = vdupq_n_f32(globalConfig.samplePeriod);
        return freq(f0) * sp;
      }

      inline float32x4_t deltaOffset(const float32x4_t f0, const Vpo& offset) const {
        return delta(offset.freq(f0));
      }

      inline void configure(const float32x4_t vpo) {
        mScale.set(util::simd::exp_f32(util::four::fscale_vpo(vpo)));
      }

      TrackAndHold mScale { 1 };
    };

    class Pitch {
      public:
        inline Pitch() {}
        inline Pitch(float32x4_t o, float32x4_t c) : mOctave(o), mCents(c) { }

        static inline Pitch from(float32x4_t value) {
          auto voltage = toVoltage(value);
          auto octave  = toOctave(voltage);
          return Pitch { octave, toCents(voltage - octave) };
        }

        inline float32x4_t octave() const { return mOctave; }
        inline float32x4_t cents()  const { return mCents; }

        inline Pitch atCents(float32x4_t cents) const {
          return Pitch(mOctave, cents);
        }

        inline Pitch atCents(float cents) const {
          return atCents(vdupq_n_f32(cents));
        }

        inline float32x4_t value() const {
          return fromVoltage(mOctave + fromCents(mCents));
        }

      private:
        float32x4_t mOctave = vdupq_n_f32(0);
        float32x4_t mCents = vdupq_n_f32(0);
    };
  }
}