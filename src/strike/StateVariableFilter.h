#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include <util.h>
#include <svf.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace strike {

  #define STRIKE_FILTER_MODE_LOWPASS 1
  #define STRIKE_FILTER_MODE_HIGHPASS 2
  #define STRIKE_FILTER_MODE_BANDPASS 3

  class StateVariableFilter : public od::Object {
    public:
      StateVariableFilter(bool stereo) {
        mChannelCount = stereo ? 2 : 1;

        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });

          mFilter.push_back(svf::simd::Filter {});
        }

        addInput(mVpo);
        addInput(mF0);
        addInput(mGain);
        addInput(mQ);
        addInput(mMix);

        addOption(mMode);
      }

      virtual ~StateVariableFilter() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processType() {
        if (mChannelCount == 1) processTypeChannel<1>();
        else                    processTypeChannel<2>();
      }

      template <int CH>
      inline void processTypeChannel() {
        float *in[CH], *out[CH];
        svf::simd::Filter *filter[CH];
        for (int channel = 0; channel < CH; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
          filter[channel] = &mFilter.at(channel);
        }

        float *vpo  = mVpo.buffer();
        float *f0   = mF0.buffer();
        float *gain = mGain.buffer();
        float *q    = mQ.buffer();
        float *mix  = mMix.buffer();

        const util::simd::clamp sClpQ    { 0.707107f, 100.0f };
        const util::simd::clamp sClpGain { 1.0, 100.0 };;
        const util::simd::vpo   sVpo     { };

        const svf::simd::Constants sBc { globalConfig.sampleRate };

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto vMix  = vld1q_f32(mix + i);
          auto vGain = sClpGain.lowBase(vld1q_f32(gain + i));

          svf::simd::Coefficients cf {
            sBc,
            sVpo.process(vld1q_f32(vpo + i), vld1q_f32(f0 + i)),
            sClpQ.lowBase(vld1q_f32(q + i))
          };

          for (int c = 0; c < CH; c++) {
            auto _in = vld1q_f32(in[c] + i);
            auto _out = filter[c]->process(cf, _in * vGain, vMix);
            vst1q_f32(out[c] + i, util::simd::tanh(_out));
          }
        }
      }

      od::Inlet mVpo  { "V/Oct" };
      od::Inlet mF0   { "Fundamental" };
      od::Inlet mGain { "Gain" };
      od::Inlet mQ    { "Resonance" };
      od::Inlet mMix  { "Mix" };

      od::Option mMode { "Mode", STRIKE_FILTER_MODE_LOWPASS };
#endif

    private:
      int mChannelCount = 1;
      std::vector<svf::simd::Filter> mFilter;
  };
}
