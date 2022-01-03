#pragma once

#include <od/config.h>
#include <od/constants.h>
#include <od/objects/Object.h>

#include <util/math.h>

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

          float32x4_t clampVpo = vmaxq_f32(negOne, vminq_f32(one, loadVpo));
          float32x4_t tune     = simd_exp(clampVpo * glog2);
          float32x4_t delta    = loadFreq * sp * tune;

          auto p = vdupq_n_f32(mPhase) + delta * cScale;

          uint32_t _sync[4], _syncDelta[4];
          vst1q_u32(_sync, vcgtq_f32(loadSync, vdupq_n_f32(0)));
          vst1q_u32(_syncDelta, vreinterpretq_u32_f32(p));

          uint32_t _offset[4], _o = 0;
          for (int i = 0; i < 4; i++) {
            if (_sync[i]) { _o = _syncDelta[i]; }
            _offset[i] = _o;
          }

          p = p - vreinterpretq_f32_u32(vld1q_u32(_offset));
          p = p - util::four::floor(p);
          mPhase = vgetq_lane_f32(p, 3);

          auto final = vbslq_f32(vcltq_f32(p, loadWidth), negOne, one);
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
      const float32x4_t cScale = util::four::make(1, 2, 3, 4);
      float mPhase = 0.0f;
  };
}
