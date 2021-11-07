#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <util.h>
#include <filter.h>

namespace strike {

  class Sieve : public od::Object {
    public:
      Sieve() {
        addInput(mInLeft);
        addInput(mInRight);

        addInput(mVpo);
        addInput(mF0);
        addInput(mGain);
        addInput(mQ);
        addInput(mMix);

        addOutput(mOutLeft);
        addOutput(mOutRight);
      }

      virtual ~Sieve() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *inLeft  = mInLeft.buffer();
        const float *inRight = mInRight.buffer();

        const float *vpo  = mVpo.buffer();
        const float *f0   = mF0.buffer();
        const float *gain = mGain.buffer();
        const float *q    = mQ.buffer();
        const float *mix  = mMix.buffer();

        float *outLeft = mOutLeft.buffer();
        float *outRight = mOutRight.buffer();

        //float32x2_t last = vdup_n_f32(0);

        for (int i = 0; i < FRAMELENGTH; i++) {
          mMixer.configure(vld1_dup_f32(mix + i));
          mFilter.configure(vld1_dup_f32(f0 + i), vld1_dup_f32(vpo + i), vld1_dup_f32(q + i));

          auto _in = util::two::make(inLeft[i], inRight[i]);
          _in = vmul_f32(_in, vld1_dup_f32(gain + i));

          auto _out = mFilter.processAndMix(mMixer, _in);
          //_out = util::two::tanh(_out);
          outLeft[i] = vget_lane_f32(_out, 0);
          outRight[i] = vget_lane_f32(_out, 1);

          // if (i > 0 && i % 4 == 0) {
          //   auto l = vld1q_f32(outLeft + i - 4);
          //   auto r = vld1q_f32(outRight + i - 4);

          // }

          // if (i % 2 == 1) {
          //   auto lim = util::simd::tanh(vcombine_f32(last, _out));
          //    outLeft[i - 1] = vgetq_lane_f32(lim, 0);
          //   outRight[i - 1] = vgetq_lane_f32(lim, 1);
          //    outLeft[i]     = vgetq_lane_f32(lim, 2);
          //   outRight[i]     = vgetq_lane_f32(lim, 3);
          // }

          // last = _out;
        }
      }

      od::Inlet mInLeft  { "In1" };
      od::Inlet mInRight { "In2" };

      od::Inlet mVpo  { "V/Oct" };
      od::Inlet mF0   { "Fundamental" };
      od::Inlet mGain { "Gain" };
      od::Inlet mQ    { "Resonance" };
      od::Inlet mMix  { "Mix" };

      od::Outlet mOutLeft  { "Out1" };
      od::Outlet mOutRight { "Out2" };
#endif

    private:
      filter::svf::two::Filter mFilter;
      util::two::ThreeWayMix mMixer { 0, 1, 0.5 };
  };
}
