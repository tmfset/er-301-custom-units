#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <osc.h>

namespace strike {
  class Fin : public od::Object {
    public:
      Fin() {
        addOutput(mOut);

        addInput(mVpo);
        addInput(mFreq);
        addInput(mSync);
        addInput(mWidth);
        addInput(mGain);
        addInput(mBend);

        addOption(mBendMode);
      }

      virtual ~Fin() { }

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

        osc::shape::BendMode bendMode = static_cast<osc::shape::BendMode>(mBendMode.value());

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _vpo   = vld1q_f32(vpo   + i);
          auto _f0    = vld1q_f32(freq  + i);
          auto _width = vld1q_f32(width + i);
          auto _sync  = vld1q_f32(sync  + i);
          auto _gain  = vld1q_f32(gain  + i);
          auto _bend  = vld1q_f32(bend  + i);

          auto f = osc::Frequency::vpoWidth(_vpo, _f0, _width);
          auto b = osc::shape::Bend { bendMode, _bend };
          auto o = mOscillator.process<osc::shape::FIN_SHAPE_POW3>(f, b, _sync);

          vst1q_f32(out + i, vmlaq_f32(vnegq_f32(_gain), _gain, o + o));
        }
      }

      od::Outlet mOut      { "Out" };

      od::Inlet  mVpo      { "V/Oct" };
      od::Inlet  mFreq     { "Frequency" };
      od::Inlet  mSync     { "Sync" };
      od::Inlet  mWidth    { "Width" };
      od::Inlet  mGain     { "Gain" };
      od::Inlet  mBend     { "Bend" };
      od::Option mBendMode { "Bend Mode", osc::shape::BEND_INVERTED };
#endif
    private:
      osc::Fin mOscillator;
  };
}