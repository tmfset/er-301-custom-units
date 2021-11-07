#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <osc.h>

namespace strike {
  #define FORMANT_FIXED 1
  #define FORMANT_FREE 2

  class Formant : public od::Object {
    public:
      Formant() {
        addOutput(mOut);

        addInput(mVpo);
        addInput(mFreq);
        addInput(mSync);
        addInput(mFormant);
        addInput(mGain);
        addInput(mBarrel);

        addOption(mFixed);
      }

      virtual ~Formant() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        float *out = mOut.buffer();

        const float *vpo   = mVpo.buffer();
        const float *freq  = mFreq.buffer();
        const float *sync  = mSync.buffer();
        const float *formant = mFormant.buffer();
        const float *gain  = mGain.buffer();
        const float *barrel  = mBarrel.buffer();

        float fixed = 1.0f;
        if (mFixed.value() == FORMANT_FREE) {
          fixed = 0.0f;
        }
        const auto _fixed = vcgtq_f32(vdupq_n_f32(fixed), vdupq_n_f32(0));

        // Adjust the gain so that it looks approximately correct.
        const auto adjust = vdupq_n_f32(1.24f);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto o = mOscillator.process(
            vld1q_f32(freq    + i),
            vld1q_f32(vpo     + i),
            vld1q_f32(formant + i),
            vld1q_f32(barrel  + i),
            vld1q_f32(sync    + i),
            _fixed
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
      od::Inlet  mFormant  { "Formant" };
      od::Inlet  mGain     { "Gain" };
      od::Inlet  mBarrel   { "Barrel" };

      od::Option mFixed    { "Fixed", FORMANT_FIXED };
#endif
    private:
      osc::Formant mOscillator;
  };
}