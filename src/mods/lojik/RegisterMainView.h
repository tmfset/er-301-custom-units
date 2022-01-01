#pragma once

#include <od/graphics/Graphic.h>
#include <od/graphics/controls/Readout.h>
#include <math.h>
#include <HasChartData.h>
#include <util.h>
#include <graphics.h>
#include <slew.h>
#include <Register2.h>

namespace lojik {
  class RegisterMainView : public od::Graphic {
    public:
      RegisterMainView(Register2 &data, int left, int bottom, int width, int height);

      virtual ~RegisterMainView() { }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::lbwh_raw(
          v2d::of(mWorldLeft, mWorldBottom),
          v2d::of(mWidth, mHeight)
        );

        auto interior = world.inner(2);

        auto top = interior.divideTop(0.5);
        auto bottom = interior.divideBottom(0.5).withHeight(9).quantizeCenter();

        //mCircleChart.draw(fb, left.scaleHeight(0.75));

        mKeyboard.draw(fb, top);

        mChart.draw(fb, bottom, 3, 2);

        auto size = 8;

        auto left = interior.splitLeft(0.5);
        auto right = interior.splitRight(0.5);

        auto lc = left.bottomCenter().quantize();
        mOffsetReadout.update(size);
        mOffsetReadout.draw(fb, WHITE, lc.offsetX(-2), RIGHT_BOTTOM, mHighlightOffset);

        mShiftReadout.update(size);
        mShiftReadout.draw(fb, WHITE, lc.offsetX(2), LEFT_BOTTOM, mHighlightShift);

        auto rc = right.bottomCenter().quantize();
        mLengthReadout.update(size);
        mLengthReadout.draw(fb, WHITE, rc.offsetX(-2), RIGHT_BOTTOM, mHighlightLength);

        mStrideReadout.update(size);
        mStrideReadout.draw(fb, WHITE, rc.offsetX(2), LEFT_BOTTOM, mHighlightStride);
      }

      void setOffsetHighlight(bool v) { mHighlightOffset = v; }
      void setShiftHighlight(bool v)  { mHighlightShift  = v; }
      void setLengthHighlight(bool v) { mHighlightLength = v; }
      void setStrideHighlight(bool v) { mHighlightStride = v; }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::HKeyboard mKeyboard;

      graphics::Readout mOffsetReadout;
      graphics::Readout mShiftReadout;
      graphics::Readout mLengthReadout;
      graphics::Readout mStrideReadout;

      bool mHighlightOffset = false;
      bool mHighlightShift  = false;
      bool mHighlightLength = false;
      bool mHighlightStride = false;
  };
}