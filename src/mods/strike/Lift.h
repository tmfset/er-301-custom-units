#pragma once

#include <od/objects/Object.h>
#include <sstream>
#include <slew.h>
#include <filter.h>

namespace strike {
  class Lift : public od::Object {
    public:
      Lift() {
        addInput(mGate);
        addInput(mInLeft);
        addInput(mInRight);

        addParameter(mRise);
        addParameter(mFall);
        addParameter(mHeight);

        addOutput(mOutLeft);
        addOutput(mOutRight);
        addOutput(mEnv);
      }

      virtual ~Lift() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        const float *gate = mGate.buffer();
        const float *inLeft = mInLeft.buffer();
        const float *inRight = mInRight.buffer();

        float *outLeft = mOutLeft.buffer();
        float *outRight = mOutRight.buffer();
        float *env = mEnv.buffer();

        const auto height = vdup_n_f32(mHeight.value());

        mSlew.setRiseFall(
          mRise.value(),
          mFall.value()
        );

        for (int i = 0; i < FRAMELENGTH; i++) {
          auto _gate = vld1_dup_f32(gate + i);
          auto slew = mSlew.process(_gate);
          env[i] = vget_lane_f32(slew, 0);

          auto f0 = vmul_f32(slew, height);
          mFilter.setFrequency(f0);

          auto _in = util::simd::make_f32(inLeft[i], inRight[i]);
          auto _out = mFilter.process(vmul_f32(_in, slew));

          outLeft[i] = vget_lane_f32(_out, 0);
          outRight[i] = vget_lane_f32(_out, 1);
        }
      }

      od::Inlet mGate    { "Gate" };
      od::Inlet mInLeft  { "In1" };
      od::Inlet mInRight { "In2" };

      od::Parameter mRise   { "Rise", 0.01 };
      od::Parameter mFall   { "Fall", 0.2 };
      od::Parameter mHeight { "Height", 1 };

      od::Outlet mOutLeft  { "Out1" };
      od::Outlet mOutRight { "Out2" };
      od::Outlet mEnv      { "Env" };
#endif

    private:
      slew::two::Slew mSlew;
      filter::onepole::two::Lowpass mFilter;
  };
}