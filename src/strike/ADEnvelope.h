#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <env.h>
#include <shape.h>

namespace strike {
  class ADEnvelope : public od::Object {
    public:
      ADEnvelope() {
        addInput(mIn);
        addOutput(mOut);

        addInput(mRise);
        addInput(mFall);
        addInput(mLoop);
        addInput(mHeight);
        addInput(mBendUp);
        addInput(mBendDown);
      }

      virtual ~ADEnvelope() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *in = mIn.buffer();
        float *out = mOut.buffer();

        const float *rise     = mRise.buffer();
        const float *fall     = mFall.buffer();
        const float *loop     = mLoop.buffer();
        const float *height   = mHeight.buffer();
        const float *bendUp   = mBendUp.buffer();
        const float *bendDown = mBendDown.buffer();

        env::simd::Frequency f;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          f.setRiseFall(vld1q_f32(rise + i), vld1q_f32(fall + i));

          auto o = mEnvelope.process(
            vld1q_f32(in + i),
            vld1q_f32(loop + i),
            f,
            shape::simd::Bend {
              vld1q_f32(bendUp + i),
              vld1q_f32(bendDown + i),
            }
          );

          auto h = vld1q_f32(height + i);

          vst1q_f32(out + i, o * h);
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Inlet mRise     { "Rise" };
      od::Inlet mFall     { "Fall" };
      od::Inlet mLoop     { "Loop" };
      od::Inlet mHeight   { "Height" };
      od::Inlet mBendUp   { "Bend Up" };
      od::Inlet mBendDown { "Bend Down" };
#endif
    private:
      env::simd::AD mEnvelope;
  };
}