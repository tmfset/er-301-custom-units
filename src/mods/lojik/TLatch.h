#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <sense.h>
#include <util.h>

namespace lojik {
  class TLatch : public od::Object {
    public:
      TLatch() {
        addInput(mIn);
        addInput(mDuration);
        addInput(mReset);
        addOutput(mOut);

        addOption(mSense);
      }

      virtual ~TLatch() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in       = mIn.buffer();
        float *duration = mDuration.buffer();
        float *reset    = mReset.buffer();
        float *out      = mOut.buffer();

        auto sense = vdupq_n_f32(getSense(mSense));
        auto zero  = vdupq_n_f32(0.0f);
        auto sp    = vdupq_n_f32(globalConfig.samplePeriod);

        float _phase = mPhase;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          float32x4_t _in       = vld1q_f32(in + i);
          float32x4_t _duration = vld1q_f32(duration + i);
          float32x4_t _reset    = vld1q_f32(reset + i);

          auto highIn    = vcgtq_f32(_in, sense);
          auto highReset = vcgtq_f32(_reset, zero);

          uint32_t _highIn[4], _highReset[4];
          vst1q_u32(_highIn, highIn);
          vst1q_u32(_highReset, highReset);

          float _delta[4];
          vst1q_f32(_delta, sp * invert(vmaxq_f32(_duration, sp)));

          for (int j = 0; j < 4; j++) {
            _phase -= _delta[j];
            if (_highIn[j]) _phase = 1.0f;
            if (_highReset[j]) _phase = 0.0f;
            _delta[j] = _phase;
          }

          auto phase = vmaxq_f32(vld1q_f32(_delta), zero);
          _phase = vgetq_lane_f32(phase, 3);
          vst1q_f32(out + i, vcvtq_n_f32_u32(vcgtq_f32(phase, zero), 32));
        }

        mPhase = _phase;
      }

      od::Inlet  mIn       { "In" };
      od::Inlet  mDuration { "Duration" };
      od::Inlet  mReset    { "Reset" };
      od::Outlet mOut      { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      float mPhase = 0.0f;
  };
}