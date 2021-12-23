#pragma once

#include <od/config.h>
#include <vector>
#include "Scale.h"

namespace common {
  // static const Scale scales[] = {
  //   { "off", 0, { } }
  // };

  class ScaleBook {
    public:
      inline ScaleBook() {
        //addPitch(0);
        //addPitch(100);
        //addPitch(200);
        //addPitch(300);
        //addPitch(400);
        //addPitch(500);
        //addPitch(600);
        //addPitch(700);
        //addPitch(800);
        //addPitch(900);
        //addPitch(1000);
        //addPitch(1100);
        //commitScale("off");
      }

      // void addPitch(float cents) {
      //   mWrite.push_back(cents);
      // }

      // void commitScale(std::string name) {
      //   //Scale scale { name, mWrite.size(), mWrite. };
      //   //mScales.push_back(scale);
      //   mWrite.clear();
      // }

      inline int size() const {
        return mScales.size();
      }

      const Scale& scale(int i) const {
        return mScales.at(i);
      }

    private:
      std::vector<float> mWrite;
      std::vector<Scale> mScales = {
        { "off",         0, { } },

        { "12-tet",     12, { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 } },
        { "24-tet",     24, { 0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150 } },

        { "major pent.", 5, { 0, 200, 400, 700, 900 } },
        { "minor pent.", 5, { 0, 300, 500, 700, 1000} },

        { "nat. minor",  7, { 0, 200, 300, 500, 700, 800, 1000, 1200 } },
        { "harm. minor", 7, { 0, 200, 300, 500, 700, 800, 1100, 1200 } },

        { "ionian",      7, { 0, 200, 400, 500, 700, 900, 1100 } },
        { "dorian",      7, { 0, 200, 300, 500, 700, 900, 1000 } },
        { "phrygian",    7, { 0, 100, 300, 500, 700, 800, 1000 } },
        { "lydian",      7, { 0, 200, 400, 600, 700, 900, 1100 } },
        { "mixolydian",  7, { 0, 200, 400, 500, 700, 800, 1000 } },
        { "aeolian",     7, { 0, 200, 300, 500, 700, 800, 1000 } },
        { "locrian",     7, { 0, 100, 300, 500, 600, 800, 1000 } },

        { "whole tone",  6, { 0, 200, 400, 600, 800, 1000 } },
      };
  };
}