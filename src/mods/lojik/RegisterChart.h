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
      RegisterChart(common::HasChartData &regLike, int left, int bottom, int width, int height);

      virtual ~RegisterChart() { }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world    = graphics::Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);
        auto interior = world.inner(2);

        mChart.draw(fb, interior);
      }

      graphics::HChart mChart;
  };
}