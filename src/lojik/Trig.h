#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <od/objects/Object.h>
#include <sense.h>
#include <util.h>

namespace lojik {
  class Trig : public od::Object {
    public:
      Trig() {
        addInput(mIn);
        addOutput(mOut);
        addOption(mSense);
      }

      virtual ~Trig() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        const float *in  = mIn.buffer();
        float *out = mOut.buffer();

        const auto sense = getSense(mSense);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          uint32_t trig[4];
          mLatch.readTriggersU(trig, vcgtq_f32(vld1q_f32(in + i), vdupq_n_f32(sense)));
          vst1q_f32(out + i, vcvtq_n_f32_u32(vld1q_u32(trig), 32));
        }
      }

      od::Inlet  mIn    { "In" };
      od::Outlet mOut   { "Out" };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      float mCurrent = 0.0f;
      bool mAllow = true;
      Trigger mLatch;
  };
}
