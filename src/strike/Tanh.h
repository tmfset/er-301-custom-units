#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <util.h>

namespace strike {
  class Tanh : public od::Object {
    public:
      Tanh() {
        addOutput(mOut);
        addInput(mIn);
        addInput(mGain);
      }

      virtual ~Tanh() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        float *out = mOut.buffer();
        const float *in = mIn.buffer();
        const float *gain  = mGain.buffer();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          const auto _in = vld1q_f32(in + i);
          const auto _g  = vld1q_f32(gain + i);
          vst1q_f32(out + i, util::simd::tanh(_in * _g));
        }
      }

      od::Outlet mOut  { "Out" };
      od::Inlet  mIn   { "In" };
      od::Inlet  mGain { "Gain" };
#endif
  };
}