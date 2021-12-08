#pragma once

#include "slew.h"

namespace common {
  class HasChartData {
    public:
      HasChartData();
      virtual ~HasChartData();

      virtual int length() = 0;
      virtual int current() = 0;
      virtual float value(int i) = 0;

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

      int length() { return mWrap.length(); }
      int current() { return mWrap.current(); }

      float value(int i) { return mValues[i].process(mSlewRate, mWrap.value(i)); }
      void clear(int i) { mValues[i].hardSet(0); }

      void attach() { mWrap.attach(); }
      void release() { mWrap.release(); }

    private:
      HasChartData &mWrap;
      slew::SlewRate mSlewRate;
      slew::Slew mValues[MAX];
  };
}