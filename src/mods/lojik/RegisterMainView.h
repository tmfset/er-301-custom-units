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

        mChart.draw(fb, bottom);

        auto size = 8;

        auto left = interior.splitLeft(0.5);
        auto right = interior.splitRight(0.5);

        auto leftCenter = left.bottomCenter();
        mOffsetReadout.drawRightBottom(fb, WHITE, leftCenter.offsetX(-2), size);
        mShiftReadout.drawLeftBottom(fb, WHITE, leftCenter.offsetX(2), size);

        auto rightCenter = right.bottomCenter();
        mLengthReadout.drawRightBottom(fb, WHITE, rightCenter.offsetX(-2), size);
        mStrideReadout.drawLeftBottom(fb, WHITE, rightCenter.offsetX(2), size);
      }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::HKeyboard mKeyboard;

      graphics::Readout mOffsetReadout;
      graphics::Readout mShiftReadout;
      graphics::Readout mLengthReadout;
      graphics::Readout mStrideReadout;
  };
}