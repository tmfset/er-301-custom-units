#pragma once

#include <od/config.h>
#include <vector>
#include <algorithm>
#include "util.h"

#define CENTS_PER_OCTAVE 1200.0f

namespace common {
  class Scale {
    public:
      inline Scale(std::vector<float> &pitches) {
        for (unsigned int i = 0; i < pitches.size(); i++) {
          auto cents = pitches[i];
          if (cents < 0 || cents > 1200) continue;
          mCentValues.push_back(cents);

          cents *= 1.0f / 1200.0f;
          mPitchClasses.push_back(cents);
          mPitchClasses.push_back(cents - 1.0f);
        }

        std::sort(mPitchClasses.begin(), mPitchClasses.end());

        for (unsigned int i = 1; i < mPitchClasses.size(); i++) {
          float prev = mPitchClasses[i - 1];
          float curr = mPitchClasses[i];
          mPitchBoundaries.push_back(0.5f * (prev + curr));
        }
      }

      inline int size() const {
        return mCentValues.size();
      }

      inline float getCentValue(int i) const {
        return mCentValues.at(i);
      }

    private:
      std::vector<float> mPitchBoundaries;
      std::vector<float> mPitchClasses;
      std::vector<float> mCentValues;
  };
}