#pragma once

#include <graphics/primitives/all.h>

namespace graphics {
  class ListWindow {
    public:
      static inline ListWindow from(Range window, float itemSize, float itemPad) {
        return ListWindow { window, itemSize, itemPad, 0 };
      }

      inline ListWindow scrollTo(float index, int total) const {
        auto relative = Range::fromLeft(0, relativeEnd(total - 1));
        auto window   = mWindow.atCenter(relativeCenter(index));
        auto bounded  = relative.insert(window);
        return atWindowWithOffset(bounded.atCenter(mWindow), -bounded.left());
      }

      inline float globalStartFromLeft(float i)  const { return toGlobalFromLeft(relativeStart(i)); }
      inline float globalCenterFromLeft(float i) const { return toGlobalFromLeft(relativeCenter(i)); }
      inline float globalEndFromLeft(float i)    const { return toGlobalFromLeft(relativeEnd(i)); }

      inline float globalStartFromRight(float i)  const { return toGlobalFromRight(relativeStart(i)); }
      inline float globalCenterFromRight(float i) const { return toGlobalFromRight(relativeCenter(i)); }
      inline float globalEndFromRight(float i)    const { return toGlobalFromRight(relativeEnd(i)); }

      inline bool visible(float v) const { return mWindow.contains(v); }

    private:
      inline ListWindow(Range window, float itemSize, float itemPad, float globalOffset) :
        mWindow(window),
        mItemSize(itemSize),
        mItemPad(itemPad),
        mGlobalOffset(globalOffset) { }

      inline ListWindow atWindowWithOffset(Range window, float offset) const {
        return ListWindow { window, mItemSize, mItemPad, offset };
      }

      inline float relativeStart(float i)  const { return (mItemSize + mItemPad) * i; }
      inline float relativeCenter(float i) const { return relativeStart(i) + mItemSize / 2.0f; }
      inline float relativeEnd(float i)    const { return relativeStart(i) + mItemSize; }

      inline float globalLeft()  const { return mWindow.left() + mGlobalOffset; }
      inline float globalRight() const { return mWindow.right() - mGlobalOffset; }

      inline float toGlobalFromLeft(float v)  const { return globalLeft() + v; }
      inline float toGlobalFromRight(float v) const { return globalRight() - v; }

      Range mWindow;
      float mItemSize = 0;
      float mItemPad = 0;
      float mGlobalOffset = 0;
  };
}