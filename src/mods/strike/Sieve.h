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

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          const auto cf4 = filter::svf::four::Coefficients {
            vld1q_f32(f0 + i),
            vld1q_f32(vpo + i),
            vld1q_f32(q + i)
          };

          auto _ka123 = cf4.ka123();
          float ka123[16];
          vst4q_f32(ka123, _ka123);

          float32x4_t g     = vld1q_f32(gain + i);
          float32x4_t left  = vld1q_f32(inLeft + i) * g;
          float32x4_t right = vld1q_f32(inRight + i) * g;
          float32x4x2_t _lr { left, right };

          float lr[8];
          vst2q_f32(lr, _lr);

          auto m = vld4_dup_f32(mix + i);

          auto filter = mFilter;

          for (int j = 0; j < 4; j++) {
            auto cf = filter::svf::two::Coefficients {
              vld4_dup_f32(ka123 + j * 4)
            };
            auto tw = util::two::ThreeWay::punit(m.val[j]);

            auto lri = lr + j * 2;
            auto in = vld1_f32(lri);
            auto out = filter.process(cf, tw, in);
            vst1_f32(lri, out);
          }

          mFilter = filter;

          auto olr = vld2q_f32(lr);
          left  = olr.val[0];
          right = olr.val[1];

          left  = util::four::tanh(left);
          right = util::four::tanh(right);

          vst1q_f32(outLeft + i, left);
          vst1q_f32(outRight + i, right);
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
      //util::four::ThreeWayMix mMixer { 0, 1, 0.5 };
  };
}
