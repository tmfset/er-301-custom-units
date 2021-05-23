#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include <util.h>
#include <env.h>
#include <shape.h>
#include <svf.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace strike {
  class LowPassGate : public od::Object {
    public:
      LowPassGate(bool stereo) {
        logInfo("LPG ctor");

        mChannelCount = stereo ? 2 : 1;

        mFilter.reserve(mChannelCount);

        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });

          mFilter.push_back(svf::simd::Filter {});
        }

        addOutput(mEnv);

        addInput(mTrig);
        addInput(mLoop);
        addInput(mRise);
        addInput(mFall);
        addInput(mBend);
        addInput(mHeight);

        addOption(mBendMode);
      }

      virtual ~LowPassGate() { }

#ifndef SWIGLUA
      virtual void process();

      template <int CH>
      void processChannels();

      od::Outlet mEnv { "Env" };

      od::Inlet mTrig   { "Trig" };
      od::Inlet mLoop   { "Loop" };

      od::Inlet mRise   { "Rise" };
      od::Inlet mFall   { "Fall" };
      od::Inlet mBend   { "Bend" };
      od::Inlet mHeight { "Height" };

      od::Option mBendMode { "Bend Mode", shape::BEND_NORMAL };
#endif
    private:
      int mChannelCount = 1;
      std::vector<svf::simd::Filter> mFilter;
      env::simd::AD mEnvelope;
      svf::simd::Filter mEnvelopeFilter;
  };
}