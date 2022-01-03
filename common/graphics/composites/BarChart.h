#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/composites/ListWindow.h>
#include <graphics/interfaces/all.h>
#include <graphics/primitives/all.h>

namespace graphics {
  class HChart {
    public:
      inline HChart(HasChart &data) :
          mChartData(data) {
        mChartData.attach();
      }

      inline ~HChart() {
        mChartData.release();
      }

      inline void draw(od::FrameBuffer &fb, const Box& world, int width, int pad) {
        auto fit = 16;

        width = util::fdr(world.horizontal().segment(fit, pad));
        auto inner = Range::fromSegment(fit, pad, width).atCenter(world.horizontal());

        auto length  = mChartData.getChartSize();
        auto current = mChartData.getChartCurrentIndex();
        auto base    = mChartData.getChartBaseIndex();

        auto window = ListWindow::from(inner, width, pad)
          .scrollTo(current, length);

        for (int i = 0; i < length; i++) {
          auto x = window.globalCenterFromLeft(i);
          if (!window.visible(x)) continue;

          auto value = mChartData.getChartValue(i);
          auto wh = v2d::of(width, value * world.height() / 2.0f);
          auto xy = world.center().atX(x);

          auto isCurrent = i == current;
          auto isBase    = i == base;
          Box::cbwh(xy, wh).fill(fb, isCurrent ? GRAY12 : GRAY10);
          if (isBase) Box::cs(xy, width + 2).trace(fb, GRAY5);
          if (isCurrent) Box::cs(xy, width + 2).trace(fb, WHITE);
        }
      }

    private:
      HasChart &mChartData;
  };
}