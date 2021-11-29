#pragma once

#include <od/config.h>
#include <vector>
#include <algorithm>

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

      inline float quantize(float value) {
        float V = FULLSCALE_IN_VOLTS * value;
        int octave = (int)V;
        // Remove octaves
        float pitch = V - octave;
        // pitch is now in  (-1,1)
        // Find the first quantized pitch that is greater than the candidate pitch.
        std::vector<float>::iterator i = std::upper_bound(mPitchBoundaries.begin(),
                                                          mPitchBoundaries.end(),
                                                          pitch);
        pitch = mPitchClasses[std::distance(mPitchBoundaries.begin(), i)];
        // Add octaves back in
        V = pitch + octave;
        return V * (1.0f / FULLSCALE_IN_VOLTS);
      }

    private:
      std::vector<float> mPitchBoundaries;
      std::vector<float> mPitchClasses;
      std::vector<float> mCentValues;
  };
}