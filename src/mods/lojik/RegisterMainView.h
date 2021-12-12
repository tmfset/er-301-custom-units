#pragma once

#include <od/graphics/Graphic.h>
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

        auto interior = world.inner(2).quantizeCenter();

        auto top = interior.divideTop(0.5);
        auto bottom = interior.divideBottom(0.5);

        //mCircleChart.draw(fb, left.scaleHeight(0.75));

        graphics::HKeyboard(1).draw(fb, top);

        mChart.draw(fb, bottom);
      }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
  };
}