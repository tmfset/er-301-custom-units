#pragma once

#include <ui/dial/dial.h>
#include <ui/DialMap.h>

namespace ui {
  class LinearDialMap : public DialMap {
    public:
      LinearDialMap(
        float min,
        float max,
        float zero,
        float rounding,
        bool wrap,

        float superCoarse,
        float coarse,
        float fine,
        float superFine
      );

      virtual ~LinearDialMap();

      virtual float valueAt(const ui::dial::Position &p) const {
        float result = mRange.min() + mRadix.valueAt(p);
        return util::frt(result, mRounding);
      }

      virtual ui::dial::Position positionAt(float value) const {
        if (value < mRange.min()) return mRadix.min();
        if (value > mRange.max()) return mRadix.max();
        return mRadix.positionAt(value - mRange.min());
      }

      virtual ui::dial::Position zero() const {
        return positionAt(mZero);
      }

      virtual void move(ui::dial::Position &p, int change, bool shift, bool fine) {
        mRadix.move(p, change, shift, fine, mWrap);
      }

    private:
      dial::Range mRange;
      dial::RadixSet mRadix;
      float mRounding;
      float mZero;
      bool mWrap;
  };
}