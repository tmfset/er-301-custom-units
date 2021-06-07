#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <voice.h>

namespace polygon {
  class Polygon : public od::Object {
    public:
      Polygon() {
        addOutput(mOut);
        addInput(mGate);
        addInput(mVpo);
        addInput(mDetune);
        addInput(mF0);
        addInput(mRise);
        addInput(mFall);
      }

      virtual ~Polygon() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        auto out = mOut.buffer();

        const auto gate   = mGate.buffer();
        const auto vpo    = mVpo.buffer();
        const auto detune = mDetune.buffer();
        const auto f0     = mF0.buffer();
        const auto rise   = mRise.buffer();
        const auto fall   = mFall.buffer();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          mControl.process(
            vdupq_n_u32(0),
            vcvtq_n_u32_f32(vld1q_f32(gate + i), 32),
            vld1q_f32(vpo + i),
            vld1q_f32(detune + i)
          );

          vst1q_f32(out + i, mVoice.process(
            mControl,
            vld1q_f32(f0 + i),
            vld1q_f32(rise + i),
            vld1q_f32(fall + i)
          ));
        }
      }

      od::Outlet mOut    { "Out" };
      od::Inlet  mGate   { "Gate" };
      od::Inlet  mVpo    { "V/Oct" };
      od::Inlet  mDetune { "Detune" };
      od::Inlet  mF0     { "Fundamental" };
      od::Inlet  mRise   { "Rise" };
      od::Inlet  mFall   { "Fall" };
#endif
    private:
      voice::Control mControl { 0 };
      voice::Voice mVoice;
  };
}