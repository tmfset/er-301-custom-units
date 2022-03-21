#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/composites/ListWindow.h>
#include <graphics/interfaces/all.h>
#include <graphics/primitives/all.h>

#include <dsp/slew.h>

namespace graphics {
  class Chart {
    public:
      Chart(HasChart &data) :
          mData(data) {
        mData.attach();
      }

      virtual ~Chart() {
        mData.release();
      }

    protected:
      HasChart &mData;
  };

  class HChart : protected Chart {
    public:
      HChart(HasChart &data) : Chart(data) { }
      virtual ~HChart() { }

      inline void draw(od::FrameBuffer &fb, const Box& world, int pad, int fit) {

        int width = util::fdr(world.horizontal().segment(fit, pad));
        auto inner = Range::fromSegment(fit, pad, width).alignCenter(world.horizontal());

        auto length  = mData.getChartSize();
        auto current = mData.getChartCurrentIndex();
        auto base    = mData.getChartBaseIndex();

        auto slewCurrent = mIndexSlew.process(mIndexSlewRate, current);

        auto window = ListWindow::from(inner, width, pad)
          .scrollTo(slewCurrent, length);

        for (int i = 0; i < length; i++) {
          auto x = window.globalCenterFromLeft(i);
          if (!window.visible(x)) continue;

          auto value = mData.getChartValue(i);
          auto wh = v2d::of(width, value * world.height() / 2.0f);
          auto xy = world.center().atX(x);

          auto isCurrent = i == current;
          auto isBase    = i == base;
          Box::cbwh(xy, wh).fill(fb, isCurrent ? GRAY10 : GRAY5);
          if (isBase) Box::cs(xy, width + 2).trace(fb, GRAY10);
          if (isCurrent) Box::cs(xy, width + 2).trace(fb, WHITE);
        }
      }

    private:
      slew::SlewRate mIndexSlewRate = slew::SlewRate::fromRate(0.05, GRAPHICS_REFRESH_PERIOD);
      slew::Slew mIndexSlew;
  };

  class IChart : protected Chart {
    public:
      IChart(HasChart &data) : Chart(data) { }
      virtual ~IChart() { }

      inline void draw(od::FrameBuffer &fb, const Box &world, int pad, int fit) {

        int height = util::fdr(world.vertical().segment(fit, pad));
        auto inner = Range::fromSegment(fit, pad, height).alignCenter(world.vertical());

        auto length  = mData.getChartSize();
        auto current = mData.getChartCurrentIndex();
        auto base    = mData.getChartBaseIndex();

        auto slewCurrent = mIndexSlew.process(mIndexSlewRate, current);

        auto window = ListWindow::from(inner, height, pad)
          .scrollTo(slewCurrent, length);

        for (int i = 0; i < length; i++) {
          auto y = window.globalCenterFromRight(i);
          if (!window.visible(y)) continue;

          auto value = mData.getChartValue(i);
          auto wh = v2d::of(value * world.width() / 2.0f, height);
          auto xy = world.center().atY(y);

          auto isCurrent = i == current;
          auto isBase    = i == base;
          Box::clwh(xy, wh).fill(fb, isCurrent ? GRAY12 : GRAY10);
          if (isBase) Box::cs(xy, height + 2).trace(fb, GRAY5);
          if (isCurrent) Box::cs(xy, height + 2).trace(fb, WHITE);
        }
      }

    private:
      slew::SlewRate mIndexSlewRate = slew::SlewRate::fromRate(0.05, GRAPHICS_REFRESH_PERIOD);
      slew::Slew mIndexSlew;
  };
}