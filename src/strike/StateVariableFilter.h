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
        for (int channel = 0; channel < CH; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
        }

        float *vpo  = mVpo.buffer();
        float *f0   = mF0.buffer();
        float *gain = mGain.buffer();
        float *q    = mQ.buffer();

        util::simd::clamp sClpUnit { -1.0f, 1.0f };
        util::simd::clamp sClpQ    { 0.707107f, 30.0f };
        util::simd::vpo   sVpo     { };
        util::simd::tanh  sTanh    { };

        svf::simd::Constants sBc { globalConfig.sampleRate };

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto vVpo  = vld1q_f32(vpo  + i);
          auto vF0   = vld1q_f32(f0   + i);
          auto vGain = vld1q_f32(gain + i);
          auto vQ    = vld1q_f32(q    + i);

          svf::simd::Coefficients cf {
            sBc,
            sVpo.process(sClpUnit.process(vVpo), vF0),
            sClpQ.process(sClpQ.min + vQ)
          };

          for (int c = 0; c < CH; c++) {
            auto _in = vld1q_f32(in[c] + i);
            auto _out = mFilter.at(c).process(cf, _in * vGain);
            vst1q_f32(out[c] + i, sTanh.process(_out));
          }
        }
      }

      od::Inlet  mVpo  { "V/Oct" };
      od::Inlet  mF0   { "Fundamental" };
      od::Inlet  mGain { "Gain" };
      od::Inlet  mQ    { "Resonance" };

      od::Option mMode { "Mode", STRIKE_FILTER_MODE_LOWPASS };
#endif

    private:
      int mChannelCount = 1;
      std::vector<svf::simd::Filter> mFilter;
  };
}
