#pragma once

#include <od/objects/Object.h>
#include <vector>
#include <sstream>
#include <compressor.h>

namespace strike {
  class Lift : public od::Object {
    public:
      Lift(bool stereo) {
        mChannelCount = stereo ? 2 : 1;

        mFilters.reserve(mChannelCount);
        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });

          mFilters.push_back(filter::onepole::Filter {});
        }

        addOutput(mEnv);

        addInput(mGate);
        addParameter(mRise);
        addParameter(mFall);
        addParameter(mHeight);
      }

      virtual ~Lift() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        int cc = mChannelCount;
        float *in[cc], *out[cc];
        filter::onepole::Filter *filter[cc];
        for (int channel = 0; channel < cc; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
          filter[channel] = &mFilters.at(channel);
        }

        float *env = mEnv.buffer();

        const float *gate = mGate.buffer();

        const float rise  = mRise.value();
        const float fall  = mFall.value();
        const auto height = vdupq_n_f32(mHeight.value());

        mSlew.setRiseFall(rise, fall);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _gate = vld1q_f32(gate + i);
          auto slew = mSlew.process(_gate);
          vst1q_f32(env + i, slew);

          auto f0 = slew * height;

          for (int c = 0; c < cc; c++) {
            filter[c]->setFrequency(f0);
            auto _in = vld1q_f32(in[c] + i) * slew;
            vst1q_f32(out[c] + i, filter[c]->process(_in));
          }
        }
      }

      od::Inlet mGate { "Gate" };

      od::Parameter mRise   { "Rise", 0.01 };
      od::Parameter mFall   { "Fall", 0.2 };
      od::Parameter mHeight { "Height", 1 };

      od::Outlet mEnv { "Env" };
#endif

    private:
      int mChannelCount = 1;
      compressor::Slew mSlew;
      std::vector<filter::onepole::Filter> mFilters;
  };
}