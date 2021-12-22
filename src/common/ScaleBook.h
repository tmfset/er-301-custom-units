#pragma once

#include <od/config.h>
#include <vector>
#include "Scale.h"

namespace common {
  class ScaleBook {
    public:
      inline ScaleBook() {
        //addPitch(0);
        //addPitch(100);
        //addPitch(200);
        addPitch(300);
        //addPitch(400);
        //addPitch(500);
        //addPitch(600);
        addPitch(700);
        //addPitch(800);
        //addPitch(900);
        addPitch(1000);
        //addPitch(1100);
        commitScale();
      }

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

      const Scale& scale(int i) const {
        return mScales.at(i);
      }

    private:
      std::vector<float> mWrite;
      std::vector<Scale> mScales;
  };
}