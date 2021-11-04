#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include "util.h"
#include <filter.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace strike {

  class Sieve : public od::Object {
    public:
      Sieve(bool stereo) {
        mChannelCount = stereo ? 2 : 1;

        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });

          mFilter.push_back(filter::svf::Filter {});
        }

        addInput(mVpo);
        addInput(mF0);
        addInput(mGain);
        addInput(mQ);
        addInput(mMix);
      }

      virtual ~Sieve() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        int cc = mChannelCount;
        float *in[cc], *out[cc];
        filter::svf::Filter *filter[cc];
        for (int channel = 0; channel < cc; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
          filter[channel] = &mFilter.at(channel);
        }

        const float *vpo  = mVpo.buffer();
        const float *f0   = mF0.buffer();
        const float *gain = mGain.buffer();
        const float *q    = mQ.buffer();
        const float *mix  = mMix.buffer();

        filter::svf::Mix m;
        filter::svf::Coefficients cf;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          m.update(vld1q_f32(mix + i));
          cf.update(vld1q_f32(vpo + i), vld1q_f32(f0 + i), vld1q_f32(q + i));

          const auto g = vld1q_f32(gain + i);
          for (int c = 0; c < cc; c++) {
            filter[c]->process(cf, vld1q_f32(in[c] + i) * g);
            vst1q_f32(out[c] + i, util::simd::tanh(filter[c]->mix(m)));
          }
        }
      }

      od::Inlet mVpo  { "V/Oct" };
      od::Inlet mF0   { "Fundamental" };
      od::Inlet mGain { "Gain" };
      od::Inlet mQ    { "Resonance" };
      od::Inlet mMix  { "Mix" };
#endif

    private:
      int mChannelCount = 1;
      std::vector<filter::svf::Filter> mFilter;
  };
}
