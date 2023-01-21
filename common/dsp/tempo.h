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
        return 1.0f;
      }

      return 0.0f;
    }

    float mThreshold = 0;
    uint32_t mHysteresis = 2;

    uint32_t mCounter = 0;
    uint32_t mPhase = 0;
    uint32_t mPeriod = 44000;
  };
}