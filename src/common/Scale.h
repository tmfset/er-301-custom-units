#pragma once

#include <od/config.h>
#include <vector>
#include <algorithm>
#include <string>
#include "util.h"

namespace common {
  // class Scale {
  //   public:
  //     inline Scale(std::string name, std::vector<float> &pitches) : mName(name) {
  //       for (unsigned int i = 0; i < pitches.size(); i++) {
  //         auto cents = pitches[i];
  //         if (cents < 0 || cents > 1200) continue;
  //         mCentValues.push_back(cents);
  //       }
  //     }

  //     inline std::string name() const {
  //       return mName;
  //     }

  //     inline bool isEmpty() const {
  //       return size() == 0;
  //     }

  //     inline int size() const {
  //       return mCentValues.size();
  //     }

  //     inline float getCentValue(int i) const {
  //       return mCentValues[i];
  //     }

  //   private:
  //     std::string mName;
  //     std::vector<float> mCentValues;
  // };

  struct Scale {
    inline std::string name() const { return mName; }
    inline bool isEmpty() const { return size() == 0; }
    inline int size() const { return mSize; }
    inline float getCentValue(int i) const { return mCentValues[i]; }

    std::string mName;
    int mSize;
    float mCentValues[24];
  };
}