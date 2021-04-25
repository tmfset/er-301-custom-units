#include <Biquad.h>

#include <od/constants.h>
#include <od/config.h>
#include <hal/simd.h>
#include <util.h>

#include <math.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace strike {
  void Biquad::process() {
    float *in = mIn.buffer();
    float *out = mOut.buffer();

    float *res   = mQ.buffer();
    float *value = mValue.buffer();


    
    float scale = 1000.0f;
    float32x4_t fScale = vdupq_n_f32(scale * 2.0f * M_PI / (float)globalConfig.sampleRate);
    float32x4_t sp = vdupq_n_f32(globalConfig.samplePeriod);
    //float32x4_t one  = vdupq_n_f32(1.0f);
    //float32x4_t half = vdupq_n_f32(0.5f);
    // float32x2_t zero = vdup_n_f32(0.0f);
    // float32x4_t zeroq = vdupq_n_f32(0.0f);
    //float32x4_t minusOne = vdupq_n_f32(-1.0f);
    //float32x4_t minusTwo = vdupq_n_f32(-2.0f);

    // ---

    // float32x4_t test = zeroq;
    // test = vsetq_lane_f32(1.0f, test, 0);
    // test = vsetq_lane_f32(2.0f, test, 1);
    // test = vsetq_lane_f32(3.0f, test, 2);
    // test = vsetq_lane_f32(4.0f, test, 3);

    // float t[4];
    // vst1q_f32(t, test);
    // logDebug(1, "%f %f %f %f", t[0], t[1], t[2], t[3]);

    // float32x4x2_t bleft  = vzipq_f32(test, test);
    // float32x4x2_t bright = vzipq_f32(test, zeroq);
    // float32x4x2_t aleft  = vzipq_f32(test, test);

    // float32x4_t b[4], a[4];
    // b[0] = vcombine_f32(vget_low_f32(bleft.val[0]), vget_low_f32(bright.val[0]));
    // b[1] = vcombine_f32(vget_high_f32(bleft.val[0]), vget_high_f32(bright.val[0]));
    // b[2] = vcombine_f32(vget_low_f32(bleft.val[1]), vget_low_f32(bright.val[1]));
    // b[3] = vcombine_f32(vget_high_f32(bleft.val[1]), vget_high_f32(bright.val[1]));

    // a[0] = vcombine_f32(vget_low_f32(aleft.val[0]), zero);
    // a[1] = vcombine_f32(vget_high_f32(aleft.val[0]), zero);
    // a[2] = vcombine_f32(vget_low_f32(aleft.val[1]), zero);
    // a[3] = vcombine_f32(vget_high_f32(aleft.val[1]), zero);

    // for (int i = 0; i < 4; i++) {
    //   test = b[i];
    //   vst1q_f32(t, test);
    //   logDebug(1, "b%d - %f %f %f %f", i, t[0], t[1], t[2], t[3]);

    //   test = a[i];
    //   vst1q_f32(t, test);
    //   logDebug(1, "a%d - %f %f %f %f", i, t[0], t[1], t[2], t[3]);
    // }

    // ---

    // test = push_lane_f32(test, 6.0f);
    // vst1q_f32(t, test);
    // logDebug(1, "%f %f %f %f", t[0], t[1], t[2], t[3]);




    simd_constants constants;
    simd_tanh_constants tanh_constants;

    // float32x4_t bMask = vld1q_f32({ 0.5, 1, 0.5, 0 });
    // float32x4_t aMask = vld1q_f32({ 0, -2, 1, 0});

    

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t q     = sp + vld1q_f32(res + i);
      float32x4_t f     = fScale * vld1q_f32(value + i);

      simd_biquad_coefficients biquad_coefficients { f, q, constants };

      float32x4_t o = mBiquad.process(in, i, biquad_coefficients, constants);

      vst1q_f32(out + i, simd_tanh(o, tanh_constants));
    }

    // for (int i = 0; i < FRAMELENGTH; i += 4) {
    //   float32x4_t q     = vld1q_f32(res + i);
    //   float32x4_t qInv  = simd_invert(q + sp);
    //   float32x4_t f     = vld1q_f32(value + i);
    //   float32x4_t theta = fScale * f;

    //   float32x4_t sinTheta, cosTheta;
    //   simd_sincos(theta, &sinTheta, &cosTheta);

    //   float32x4_t beta1 = one - cosTheta;
    //   float32x4_t beta0 = beta1 * half;
    //   float32x4_t beta2 = beta0;

    //   float32x4_t alpha = sinTheta * half * qInv;
      
    //   float32x4_t alpha0 = one + alpha;
    //   float32x4_t alpha1 = minusTwo * cosTheta;
    //   float32x4_t alpha2 = one - alpha;

    //   float32x4_t alpha0Inv = simd_invert(alpha0);

    //   float b0[4], b1[4], b2[4], a0i[4], a1[4], a2[4];
    //   vst1q_f32(b0, beta0);
    //   vst1q_f32(b1, beta1);
    //   vst1q_f32(b2, beta2);
    //   vst1q_f32(a0i, alpha0Inv);
    //   vst1q_f32(a1, alpha1);
    //   vst1q_f32(a2, alpha2);

    //   simd_tanh_constants tanh_constants;

    //   for (int j = 0; j < 4; j++) {
    //     // float scale = a0i[j];

    //     // float32x4_t b = vdupq_n_f32(0);
    //     // b = vsetq_lane_f32(b0[j], b, 0);
    //     // b = vsetq_lane_f32(b1[j], b, 1);
    //     // b = vsetq_lane_f32(b2[j], b, 2);

    //     // float32x4_t a = vdupq_n_f32(0);
    //     // a = vsetq_lane_f32(a1[j], a, 1);
    //     // a = vsetq_lane_f32(a2[j], a, 2);

    //     //float32x4_t x = vld1q_f32(in  + i - 3);
    //     //float32x4_t y = vld1q_f32(out + i - 3);

    //     //float32x4_t bx = b * x;
    //     //float32x4_t ax = minusOne * a * y;

    //     out[i + j] = (
    //       b0[j] * in[i + j] +
    //       b1[j] * mInZ1 +
    //       b2[j] * mInZ2 -
    //       a1[j] * mOutZ1 -
    //       a2[j] * mOutZ2
    //     ) * a0i[j];

    //     if ((i + j) % 200 == 0) {
    //       logDebug(1, "%f %f %f %f %f %f %f %f", in[i + j], out[i + j], b0[j], b1[j], b2[j], a1[j], a2[j], a0i[j]);
    //     }

    //     mInZ2 = mInZ1;
    //     mInZ1 = in[i + j];

    //     mOutZ2 = mOutZ1;
    //     mOutZ1 = out[i + j];
    //   }

    //   vst1q_f32(out + i, simd_tanh(vld1q_f32(out + i), tanh_constants));
    // }

    // for (int i = 0; i < FRAMELENGTH; i++) {
    //   float Q = res[i] + 0.001;
    //   float v = value[i];

    //   float freq = v * 1000.0f;

    //   float theta = 2.0f * M_PI * freq / (float)globalConfig.sampleRate;

    //   float sinTheta = sin(theta);
    //   float cosTheta = cos(theta);

    //   float alpha = sinTheta * Q; // / 2*Q

    //   float beta0 = (1.0f - cosTheta) / 2.0f; // (1.0f - cosTheta) * Q / sinTheta
    //   float beta1 = 1.0f - cosTheta;          // (1.0f - cosTheta) * 2Q / sinTheta
    //   float beta2 = (1.0f - cosTheta) / 2.0f; // (1.0f - cosTheta) * Q / sinTheta

    //   float alpha0 = 1.0f + alpha;
    //   float alpha1 = -2.0f * cosTheta;
    //   float alpha2 = 1.0f - alpha;

    //   out[i] = (beta0 * in[i] +
    //            beta1 * mInZ1 +
    //            beta2 * mInZ2 -
    //            alpha1 * mOutZ1 -
    //            alpha2 * mOutZ2) / alpha0;

    //   // if (i % 200 == 0) {
    //   //   logDebug(1, "%f %f", in[i], alpha);
    //   // }
      
    //   mInZ2 = mInZ1;
    //   mInZ1 = in[i];

    //   mOutZ2 = mOutZ1;
    //   mOutZ1 = out[i];
    // }
  }
}