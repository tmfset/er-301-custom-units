#pragma once

#include <od/config.h>
#include <util/math.h>

namespace dsp {
  struct TapTempoClock {
    uint32_t process(uint32_t signal) {
      mPhase++;
      mCounter++;

      if (signal > mThreshold) {
        if (util::abs(mCounter - mPeriod) > mHysteresis) {
          mPeriod = mCounter;
        }
        mCounter = 0;
      }

      if (mPhase >= mPeriod) {
        mPhase = 0;
        return util::bcvt(true);
      }

      return 0.0f;
    }

    float mThreshold = 0;
    int mHysteresis = 2;

    int mCounter = 0;
    int mPhase = 0;
    int mPeriod = 24000;
  };

  struct TapTempo {
    void process(uint32_t signal) {
      mCounter++;
      if (signal > 0) {
        if (util::abs(mCounter - mPeriod) > mHysteresis) {
          mPeriod = mCounter;
        }
        mCounter = 0;
      }
    };

    int mHysteresis = 2;
    int mCounter = 0;
    int mPeriod = 24000;
  };
}