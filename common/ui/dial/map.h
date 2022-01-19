#pragma once

#include <util/math.h>
#include <ui/dial/dial.h>

namespace ui {
  namespace dial {
    class Map {
      public:
        virtual float valueAt(const Position &p) const = 0;
        virtual Position positionAt(float value) const = 0;
        virtual Position zero() const = 0;
        virtual void move(Position &p, int change, bool shift, bool fine) = 0;
    };

    class LinearMap : Map {
      public:
        float valueAt(const Position &p) const {
          float result = mRange.min() + mRadix.valueAt(p);
          return util::frt(result, mRounding);
        }

        Position positionAt(float value) const {
          if (value < mRange.min()) return mRadix.min();
          if (value > mRange.max()) return mRadix.max();
          return mRadix.positionAt(value - mRange.min());
        }

        Position zero() const {
          return positionAt(mZero);
        }

        void move(Position &p, int change, bool shift, bool fine) {
          mRadix.move(p, change, shift, fine, mWrap);
        }

      private:
        inline LinearMap(Range range, const Steps &steps, float rounding, float zero, bool wrap) : 
          mRange(range),
          mRadix(steps.proportionalRadixSet(range)),
          mRounding(rounding),
          mZero(zero),
          mWrap(wrap) { }

        Range mRange;
        RadixSet mRadix;
        float mRounding;
        float mZero;
        bool mWrap;
    };
  }
}