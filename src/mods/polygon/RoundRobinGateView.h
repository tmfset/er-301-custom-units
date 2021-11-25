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
      RoundRobinGateView(Observable &observable, int left, int bottom, int width, int height);
      // RoundRobinGateView(Observable &observable, int left, int bottom, int width, int height) :
      //     od::Graphic(left, bottom, width, height),
      //     mObservable(observable) {
      //   mObservable.attach();
      // }

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

        const float iCols = 1.0f / cols;
        const float iRows = 1.0f / rows;

        const float pad = 2;

        auto world  = graphics::Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);
        auto inner  = world.inner(2);

        auto corner = inner.topLeftCorner(iCols, iRows).minSquare().quantizeSize();
        auto grid   = corner.scaleDiscrete(cols, rows).centerOn(world).quantizeCenter();

        auto cell   = grid.topLeftCorner(iCols, iRows);
        auto cStep  = cell.width;
        auto rStep  = -cell.height;
        auto mark   = cell.inner(1);
        auto radius = util::fhr(mark.width * 0.5);

        for (int c = 0; c < cols; c++) {
          for (int r = 0; r < rows; r++) {
            auto box = mark.offset(cStep * c, rStep * r);

            const int index = c * rows + r;
            const float fillColor = mObservable.envLevel(index);

            auto primaryColor = pColor(index + 1);
            auto secondaryColor = sColor(index + 1);

            box.fillCircle(fb, primaryColor * fillColor);
            box.circle(fb, secondaryColor);

            if (mObservable.isVoiceArmed(index)) {
              box.center.diamond(fb, primaryColor);
            }

            if (mObservable.isVoiceNext(index)) {
              box.center.dot(fb, primaryColor);
            }
          }
        }
      }

      bool isFaded(int i) const {
        if (mCursorSelection == 0) return false;
        return mCursorSelection != i;
      }

      int pColor(int i) const {
        return isFaded(i) ? mPrimaryColor - mColorFade : mPrimaryColor;
      }

      int sColor(int i) const {
        return isFaded(i) ? mSecondaryColor - mColorFade : mSecondaryColor;
      }

      int mPrimaryColor = WHITE;
      int mSecondaryColor = GRAY10;
      int mColorFade = 5;

      int mCursorSelection = 0;

      Observable &mObservable;
  };
}