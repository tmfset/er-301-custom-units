#pragma once

#include <graphics/primitives/all.h>

namespace graphics {
  class ListWindow {
    public:
      static inline ListWindow from(Range window, float itemSize, float itemPad) {
        return ListWindow { window, itemSize, itemPad, 0 };
      }

      /**
       * Scroll to the specified index and align the list in the window.
       */
      inline ListWindow scrollTo(float index, int total, od::Justification justify = od::justifyCenter) const {
        // Calculate the full range of all items and place our window on the
        // appropriate center index.
        auto full     = Range::lw(0, relativeEnd(total - 1));
        auto window   = mWindow.atCenter(relativeCenter(index));

        // Fit the relative window
        auto bounded  = full.insert(window);
        return atWindowWithOffset(bounded.justify(mWindow, justify), -bounded.left());
      }

      inline bool hVisibleIndex(float i) const {
        return visible(globalCenterFromLeft(i));
      }

      inline Box hBox(const Box& world, float i) const {
        return Box::lbwh(
          v2d::of(globalStartFromLeft(i), world.bottom()),
          v2d::of(mItemSize, world.height())
        );
      }

      inline bool vVisibleIndex(float i) const {
        return visible(globalCenterFromRight(i));
      }

      inline Box vBox(const Box& world, float i) const {
        return Box::lbwh(
          v2d::of(world.left(), globalEndFromRight(i)),
          v2d::of(world.width(), mItemSize)
        );
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