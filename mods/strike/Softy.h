#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <dsp/osc.h>

namespace strike {
  class Softy : public od::Object {
    public:
      Softy() {
        addOutput(mOut);

        addInput(mVpo);
        addInput(mFreq);
        addInput(mSync);
        addInput(mWidth);
        addInput(mGain);
      }

      virtual ~Softy() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        float *out = mOut.buffer();

        const float *vpo   = mVpo.buffer();
        const float *freq  = mFreq.buffer();
        const float *sync  = mSync.buffer();
        const float *width = mWidth.buffer();
        const float *gain  = mGain.buffer();

        // Adjust the gain so that it looks approximately correct.
        const auto adjust = vdupq_n_f32(1.24f);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto o = mOscillator.process(
            vld1q_f32(freq  + i),
            vld1q_f32(vpo   + i),
            vld1q_f32(width + i),
            vld1q_f32(sync  + i)
          );

          auto g = vld1q_f32(gain + i);
          auto w = util::four::atan(o + o - vdupq_n_f32(1));

          vst1q_f32(out + i, w * g * adjust);
        }
      }

      od::Outlet mOut      { "Out" };

      od::Inlet  mVpo      { "V/Oct" };
      od::Inlet  mFreq     { "Frequency" };
      od::Inlet  mSync     { "Sync" };
      od::Inlet  mWidth    { "Width" };
      od::Inlet  mGain     { "Gain" };
#endif
    private:
      osc::Softy mOscillator;
  };
}