#pragma once

#include "slew.h"

namespace common {
  class HasChartData {
    public:
      HasChartData();
      virtual ~HasChartData();

      virtual int getChartSize() = 0;
      virtual int getChartCurrentIndex() = 0;
      virtual int getChartBaseIndex() = 0;
      virtual float getChartValue(int i) = 0;

      virtual void attach() = 0;
      virtual void release() = 0;
  };

  template <int MAX>
  class SlewChartData : public HasChartData {
    public:
      inline SlewChartData(HasChartData &wrap, float rate) :
          mWrap(wrap),
          mSlewRate(rate, GRAPHICS_REFRESH_PERIOD) {
        for (int i = 0; i < MAX; i++) {
          mValues[i] = slew::Slew {};
        }
      }

      virtual ~SlewChartData() { }

      int getChartSize() { return mWrap.getChartSize(); }
      int getChartCurrentIndex() { return mWrap.getChartCurrentIndex(); }
      int getChartBaseIndex() { return mWrap.getChartBaseIndex(); }

      float getChartValue(int i) { return mValues[i].process(mSlewRate, mWrap.getChartValue(i)); }
      void clearChartValue(int i) { mValues[i].hardSet(0); }

      void attach() { mWrap.attach(); }
      void release() { mWrap.release(); }

    private:
      HasChartData &mWrap;
      slew::SlewRate mSlewRate;
      slew::Slew mValues[MAX];
  };
}