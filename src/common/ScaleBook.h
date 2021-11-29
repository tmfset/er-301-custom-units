#pragma once

#include <od/config.h>
#include <vector>
#include "Scale.h"

namespace common {
  class ScaleBook {
    public:
      inline ScaleBook() { }

      void addPitch(float cents) {
        mWrite.push_back(cents);
      }

      void commitScale() {
        Scale scale { mWrite };
        mScales.push_back(scale);
        mWrite.clear();
      }

      inline int size() const {
        return mScales.size();
      }

      const Scale& getScale(int i) const {
        return mScales.at(i);
      }

    private:
      std::vector<float> mWrite;
      std::vector<Scale> mScales;
  };
}