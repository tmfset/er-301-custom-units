#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include <util.h>
#include <filter.h>

namespace strike {
  class Bique : public od::Object {
    public:
      Bique(bool stereo) {
        mChannelCount = stereo ? 2 : 1;

        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });

          mFilter.push_back(filter::biquad::Filter<4> {});
        }

        addInput(mVpo);
        addInput(mF0);
        addInput(mGain);
        addInput(mQ);

        addOption(mMode);
        addOption(mType);
      }

      virtual ~Bique() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        int cc = mChannelCount;
        float *in[cc], *out[cc];
        filter::biquad::Filter<4> *filter[cc];
        for (int channel = 0; channel < cc; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
          filter[channel] = &mFilter.at(channel);
        }

        const float *vpo  = mVpo.buffer();
        const float *f0   = mF0.buffer();
        const float *gain = mGain.buffer();
        const float *q    = mQ.buffer();

        const auto type = static_cast<filter::biquad::Type>(mType.value());
        const auto mode = static_cast<filter::biquad::Mode>(mMode.value());

        filter::biquad::Coefficients cf;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          cf.update(vld1q_f32(vpo + i), vld1q_f32(f0 + i), vld1q_f32(q + i), type);

          const auto g = vld1q_f32(gain + i);
          for (int c = 0; c < cc; c++) {
            filter[c]->process(cf, vld1q_f32(in[c] + i) * g);
            vst1q_f32(out[c] + i, util::simd::tanh(filter[c]->mode(mode)));
          }
        }
      }

      od::Inlet mVpo  { "V/Oct" };
      od::Inlet mF0   { "Fundamental" };
      od::Inlet mGain { "Gain" };
      od::Inlet mQ    { "Resonance" };

      od::Option mType { "Type", filter::biquad::LOWPASS };
      od::Option mMode { "Mode", filter::biquad::MODE_24DB };
#endif

    private:
      int mChannelCount = 1;
      std::vector<filter::biquad::Filter<4>> mFilter;
  };
}
