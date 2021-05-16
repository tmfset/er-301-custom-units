#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <env.h>

namespace strike {
  class AD : public od::Object {
    public:
      AD() {
        addInput(mIn);
        addOutput(mOut);

        addInput(mRise);
        addInput(mFall);
        addInput(mBendUp);
        addInput(mBendDown);
        addInput(mLoop);
        addInput(mHeight);
      }

      virtual ~AD() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *in  = mIn.buffer();
        float *out = mOut.buffer();

        const float *rise     = mRise.buffer();
        const float *fall     = mFall.buffer();
        const float *bendUp   = mBendUp.buffer();
        const float *bendDown = mBendDown.buffer();
        const float *loop     = mLoop.buffer();
        const float *height   = mHeight.buffer();

        env::simd::Frequency cf;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          cf.update(vld1q_f32(rise + i), vld1q_f32(fall + i));

          auto o = mEnvelope.process(
            cf,
            vld1q_f32(in + i),
            vld1q_f32(loop + i),
            vld1q_f32(bendUp + i),
            vld1q_f32(bendDown + i)
          );

          vst1q_f32(out + i, o);
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Inlet mRise     { "Rise" };
      od::Inlet mFall     { "Fall" };
      od::Inlet mBendUp   { "Bend Up" };
      od::Inlet mBendDown { "Bend Down" };
      od::Inlet mLoop     { "Loop" };
      od::Inlet mHeight   { "Height" };
#endif
    private:
      env::simd::AD mEnvelope;
  };
}