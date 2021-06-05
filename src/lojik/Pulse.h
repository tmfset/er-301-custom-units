#pragma once

#include <od/config.h>
#include <od/constants.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <math.h>

namespace lojik {
  class Pulse : public od::Object {
    public:
      Pulse() {
        addInput(mVPO);
        addInput(mFreq);
        addInput(mSync);
        addInput(mWidth);
        addInput(mGain);
        addOutput(mOut);
      }

      virtual ~Pulse() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *vpo   = mVPO.buffer();
        float *freq  = mFreq.buffer();
        float *sync  = mSync.buffer();
        float *width = mWidth.buffer();
        float *gain  = mGain.buffer();
        float *out   = mOut.buffer();

        float32x4_t negOne = vdupq_n_f32(-1.0f);
        float32x4_t one    = vdupq_n_f32(1.0f);
        float32x4_t glog2  = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
        float32x4_t sp     = vdupq_n_f32(globalConfig.samplePeriod);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          float32x4_t loadVpo   = vld1q_f32(vpo + i);
          float32x4_t loadFreq  = vld1q_f32(freq + i);
          float32x4_t loadSync  = vld1q_f32(sync + i);
          float32x4_t loadWidth = vld1q_f32(width + i);
          float32x4_t loadGain  = vld1q_f32(gain + i);

          uint32_t s[4];
          float p[4], f[4], init = mPhase;
          vst1q_u32(s, vcgtq_f32(loadSync, vdupq_n_f32(0.0f)));
          for (int i = 0, x = 1; i < 4; i++, x++) {
            if (s[i]) { init = 0; x = 0; }
            p[i] = init;
            f[i] = x;
          }

          float32x4_t clampVpo = vmaxq_f32(negOne, vminq_f32(one, loadVpo));
          float32x4_t tune     = simd_exp(clampVpo * glog2);
          float32x4_t delta    = loadFreq * sp * tune;
          auto phase = vld1q_f32(p) + vld1q_f32(f) * delta;

          phase = phase - vcvtq_f32_s32(vcvtq_s32_f32(phase));
          mPhase = vgetq_lane_f32(phase, 3);

          auto final = vbslq_f32(vcltq_f32(phase, loadWidth), one, negOne);
          vst1q_f32(out + i, final * loadGain);
        }
      }

      od::Inlet  mVPO   { "V/Oct" };
      od::Inlet  mFreq  { "Frequency" };
      od::Inlet  mSync  { "Sync" };
      od::Inlet  mWidth { "Width" };
      od::Inlet  mGain  { "Gain" };
      od::Outlet mOut   { "Out" };
#endif
    private:
      float mPhase = 0.0f;
  };
}
