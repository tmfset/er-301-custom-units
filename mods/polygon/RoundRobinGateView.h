#pragma once

#include "Observable.h"
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
#include <util/math.h>
#include <graphics/primitives/all.h>
#include <graphics/composites/Grid.h>

using namespace polygon;

namespace polygon {
  class RoundRobinGateView : public od::Graphic {
    public:
      RoundRobinGateView(Observable &observable, int left, int bottom, int width, int height);

      virtual ~RoundRobinGateView() {
        mObservable.release();
      }

      void setCursorSelection(int value) {
        mCursorSelection = value;
      }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::extractWorld(*this);
        auto grid  = graphics::Grid::create(world.pad(2), mObservable.groups(), 4, 1);

        for (int c = 0; c < grid.cols; c++) {
          for (int r = 0; r < grid.rows; r++) {
            auto box = grid.cell(c, r);

            auto index = grid.index(c, r);
            auto fillColor = mObservable.envLevel(index);

            auto primaryColor = pColor(index + 1);
            auto secondaryColor = sColor(index + 1);

            auto boxCircle = box.minCircle();
            boxCircle.fill(fb, primaryColor * fillColor);
            boxCircle.trace(fb, secondaryColor);

            auto point = graphics::Point(box.center());

            if (mObservable.isVoiceArmed(index)) {
              point.diamond(fb, primaryColor);
            }

            if (mObservable.isVoiceNext(index)) {
              point.dot(fb, primaryColor);
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