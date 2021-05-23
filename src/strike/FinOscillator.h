#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <osc.h>
#include <shape.h>

namespace strike {
  class FinOscillator : public od::Object {
    public:
      FinOscillator() {
        addOutput(mOut);

        addInput(mVpo);
        addInput(mFreq);
        addInput(mSync);
        addInput(mWidth);
        addInput(mGain);
        addInput(mBend);
        addOption(mBendMode);
      }

      virtual ~FinOscillator() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        float *out = mOut.buffer();

        const float *vpo   = mVpo.buffer();
        const float *freq  = mFreq.buffer();
        const float *sync  = mSync.buffer();
        const float *width = mWidth.buffer();
        const float *gain  = mGain.buffer();
        const float *bend  = mBend.buffer();

        shape::BendMode bendMode = (shape::BendMode)mBendMode.value();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto o = mOscillator.process(
            util::simd::vpo_scale(
              vld1q_f32(vpo + i),
              vld1q_f32(freq + i)
            ),
            vld1q_f32(width + i),
            vld1q_f32(sync + i),
            shape::simd::Bend {
              bendMode,
              vld1q_f32(bend + i)
            }
          );

          auto g = vld1q_f32(gain + i);
          auto s = vmlaq_n_f32(vdupq_n_f32(-1), o, 2.0f);

          vst1q_f32(out + i, s * g);
        }
      }

      od::Outlet mOut      { "Out" };

      od::Inlet  mVpo      { "V/Oct" };
      od::Inlet  mFreq     { "Frequency" };
      od::Inlet  mSync     { "Sync" };
      od::Inlet  mWidth    { "Width" };
      od::Inlet  mGain     { "Gain" };
      od::Inlet  mBend     { "Bend" };
      od::Option mBendMode { "Bend Mode", shape::BEND_NORMAL };
#endif
    private:
      osc::simd::Fin mOscillator;
  };
}