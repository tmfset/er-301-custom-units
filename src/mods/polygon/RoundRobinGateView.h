#pragma once

#include "Observable.h"
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
#include "util.h"
#include "graphics.h"

using namespace polygon;

namespace polygon {
  class RoundRobinGateView : public od::Graphic {
    public:
      RoundRobinGateView(Observable &observable, int left, int bottom, int width, int height) :
          od::Graphic(left, bottom, width, height),
          mObservable(observable) {
        mObservable.attach();
      }

      virtual ~RoundRobinGateView() {
        mObservable.release();
      }

      void setCursorSelection(int value) {
        mCursorSelection = value;
      }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        const float cols = (float)mObservable.groups();
        const float rows = (float)4;

        const float pad = 1.5;

        const graphics::Box world  = graphics::Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);

        const graphics::Box corner = world.divideLeft(1.0f / cols).divideTop(1.0f / rows);
        const graphics::Box inner = world.inner(
          util::fmax((world.width - corner.minDimension() * cols) / 2.0f, pad),
          util::fmax((world.height - corner.minDimension() * rows) / 2.0f, pad)
        );

        const graphics::Box column = inner.divideLeft(1.0f / cols);
        const graphics::Box row    = inner.divideTop(1.0f / rows);

        for (int c = 0; c < cols; c++) {
          const graphics::Box cBox = column.offsetX(util::fhr(column.width) * c);
          for (int r = 0; r < rows; r++) {
            const graphics::Box rBox = row.offsetY(util::fhr(-row.height) * r);

            const graphics::Box box = cBox.intersect(rBox).inner(pad);
            const int radius = util::fhr(box.minDimension() * 0.5);

            const int index = c * rows + r;
            const float fillColor = mObservable.envLevel(index);

            const int x = util::fhr(box.centerX);
            const int y = util::fhr(box.centerY);

            fb.fillCircle(pColor(index + 1) * fillColor, x, y, radius);
            fb.circle(sColor(index + 1), x, y, radius);

            if (mObservable.isVoiceArmed(index)) {
              fb.pixel(pColor(index + 1), x - 1, y);
              fb.pixel(pColor(index + 1), x + 1, y);
              fb.pixel(pColor(index + 1), x, y - 1);
              fb.pixel(pColor(index + 1), x, y + 1);
            }

            if (mObservable.isVoiceNext(index)) {
              fb.pixel(pColor(index + 1), x, y);
            }
          }
        }
      }

      bool isFaded(int i) {
        if (mCursorSelection == 0) return false;
        return mCursorSelection != i;
      }

      int pColor(int i) {
        return isFaded(i) ? mPrimaryColor - mColorFade : mPrimaryColor;
      }

      int sColor(int i) {
        return isFaded(i) ? mSecondaryColor - mColorFade : mSecondaryColor;
      }

      int mPrimaryColor = WHITE;
      int mSecondaryColor = GRAY10;
      int mColorFade = 5;

      int mCursorSelection = 0;

      Observable &mObservable;
  };
}