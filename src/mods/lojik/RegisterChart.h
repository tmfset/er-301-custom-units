#pragma once

#include <od/graphics/Graphic.h>
#include <math.h>
#include <HasChartData.h>
#include <util.h>
#include <graphics.h>
#include <slew.h>

namespace lojik {
  class RegisterChart : public od::Graphic {
    public:
      RegisterChart(common::HasChartData &data, int left, int bottom, int width, int height);

      virtual ~RegisterChart() { }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::lbwh_raw(
          v2d::of(mWorldLeft, mWorldBottom),
          v2d::of(mWidth, mHeight)
        );

        mChart.draw(fb, world.inner(2));
      }

      graphics::HChart mChart;
  };
}