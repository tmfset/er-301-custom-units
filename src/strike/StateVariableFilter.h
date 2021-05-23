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

        const float *vpo  = mVpo.buffer();
        const float *f0   = mF0.buffer();
        const float *gain = mGain.buffer();
        const float *q    = mQ.buffer();
        const float *mix  = mMix.buffer();

        const util::simd::clamp_low sClpGain { 1.0 };;
        const util::simd::exp_scale qScale   { 0.70710678118f, 1000.0f };

        svf::simd::Coefficients cf;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          const auto vMix  = vld1q_f32(mix + i);
          const auto vGain = sClpGain.lowBase(vld1q_f32(gain + i));

          const auto _f = util::simd::vpo_scale(vld1q_f32(vpo + i), vld1q_f32(f0 + i));
          const auto _q = qScale.process(vld1q_f32(q + i));
          cf.update(_f, _q, vMix);

          for (int c = 0; c < CH; c++) {
            const auto _in = vld1q_f32(in[c] + i);
            const auto _out = filter[c]->process(cf, _in * vGain);
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
