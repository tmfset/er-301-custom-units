#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/primitives/all.h>

namespace graphics {
  class CircleChart {
    public:
      inline CircleChart(HasChart &data, float size) :
          mChartData(data),
          mSize(size) {
        mChartData.attach();
      }

      inline ~CircleChart() {
        mChartData.release();
      }

      inline void draw(od::FrameBuffer &fb, const Box& world) const {
        auto length  = mChartData.getChartSize();
        auto current = mChartData.getChartCurrentIndex();

        auto outer  = world.minCircle();
        auto inner  = outer.scale(mSize);
        auto span   = (outer.radius - inner.radius) / 2.0f;
        auto center = inner.grow(span);

        inner.trace(fb, GRAY1);
        center.trace(fb, GRAY5);
        outer.trace(fb, GRAY1);

        if (length < 1) return;

        float delta = M_PI * 2.0f / (float)length;

        v2d first, prev, curr;
        for (int i = 0; i < length; i++) {
          auto next = getCirclePoint(center, span, delta, i);

          if (i == 0) first = next;
          if (i == current) curr = next;

          if (i > 0) Line::of(prev, next).trace(fb, GRAY10);
          
          prev = next;
        }

        Line::of(prev, first).trace(fb, GRAY10);

        Point::of(curr).diamond(fb, WHITE);
      }

    private:
      v2d getCirclePoint(const Circle &center, float span, float delta, int i) const {
        float amount = mChartData.getChartValue(i);
        return center.grow(span * amount).pointAtTheta(delta * (float)i);
      }

      HasChart &mChartData;
      float mSize;
  };
}