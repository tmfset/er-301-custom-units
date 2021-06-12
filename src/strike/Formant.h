#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <osc.h>

namespace strike {
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

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _vpo   = vld1q_f32(vpo   + i);
          auto _f0    = vld1q_f32(freq  + i);
          auto _formant = vld1q_f32(formant + i);
          auto _sync  = vld1q_f32(sync  + i);
          auto _gain  = vld1q_f32(gain  + i);
          auto _barrel  = vld1q_f32(barrel  + i);

          //auto f = osc::Frequency::vpoWidth(_vpo, _f0, _barrel);
          auto o = mOscillator.process(_f0, _vpo, _formant, _barrel, _sync);

          // auto s = o + o - vdupq_n_f32(1)
          // auto t = util::simd::tanh(s);
          auto g = _gain;// + _gain;
          //auto s = vmlaq_f32(vnegq_f32(g), g, o + o);
          auto w = util::simd::atan(o + o - vdupq_n_f32(1));

          vst1q_f32(out + i, w * g);
        }
      }

      od::Outlet mOut      { "Out" };

      od::Inlet  mVpo      { "V/Oct" };
      od::Inlet  mFreq     { "Frequency" };
      od::Inlet  mSync     { "Sync" };
      od::Inlet  mFormant  { "Formant" };
      od::Inlet  mGain     { "Gain" };
      od::Inlet  mBarrel   { "Barrel" };
#endif
    private:
      osc::Formant mOscillator;
  };
}