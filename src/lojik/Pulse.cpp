#include <Pulse.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <math.h>

namespace lojik {
  Pulse::Pulse() {
    addInput(mVPO);
    addInput(mFreq);
    addInput(mSync);
    addInput(mWidth);
    addOutput(mOut);

    addParameter(mPhase);
    mPhase.enableSerialization();
  }

  Pulse::~Pulse() { }

  void Pulse::process() {
    float *vpo   = mVPO.buffer();
    float *freq  = mFreq.buffer();
    float *sync  = mSync.buffer();
    float *width = mWidth.buffer();
    float *out   = mOut.buffer();

    float32x4_t negOne = vdupq_n_f32(-1.0f);
    float32x4_t zero   = vdupq_n_f32(0.0f);
    float32x4_t one    = vdupq_n_f32(1.0f);
    float32x4_t two    = vdupq_n_f32(2.0f);
    float32x4_t glog2  = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
    float32x4_t sp     = vdupq_n_f32(globalConfig.samplePeriod);

    float endPhase = mPhase.value();

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadVpo   = vld1q_f32(vpo + i);
      float32x4_t loadFreq  = vld1q_f32(freq + i);
      float32x4_t loadSync  = vld1q_f32(sync + i);
      float32x4_t loadWidth = vld1q_f32(width + i);

      uint32_t s[4];
      float p[4], f[4], init = endPhase;
      vst1q_u32(s, vcgtq_f32(loadSync, zero));
      for (int i = 0, x = 1; i < 4; i++, x++) {
        if (s[i]) { init = 0; x = 0; }
        p[i] = init;
        f[i] = x;
      }

      float32x4_t factor   = vld1q_f32(f);
      float32x4_t phase    = vld1q_f32(p);
      float32x4_t clampVpo = vmaxq_f32(negOne, vminq_f32(one, loadVpo));
      float32x4_t tune     = simd_exp(clampVpo * glog2);
      float32x4_t delta    = loadFreq * sp * tune;
      phase = vmlaq_f32(phase, factor, delta);

      float32x4_t wrap = phase - one;
      float32x4_t mask = vcvtq_n_f32_u32(vcltq_f32(wrap, zero), 32);
      phase = vmaxq_f32(phase * mask, wrap);

      float32x4_t signal = vcvtq_n_f32_u32(vcltq_f32(phase, loadWidth), 32);
      float32x4_t final  = vmlaq_f32(negOne, signal, two);
      vst1q_f32(out + i, final);

      endPhase = vgetq_lane_f32(phase, 3);
    }

    mPhase.hardSet(endPhase);
  }
}
