#pragma once

#include <od/graphics/Graphic.h>
#include <math.h>
#include <HasChartData.h>
#include <util.h>
#include <slew.h>
#include <graphics.h>

namespace lojik {
  class RegisterCircle : public od::Graphic {
    public:
      RegisterCircle(common::HasChartData &data, int left, int bottom, int width, int height);
      
      virtual ~RegisterCircle() { }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::lbwh_raw(
          v2d::of(mWorldLeft, mWorldBottom),
          v2d::of(mWidth, mHeight)
        );
        auto interior = world.inner(2);

        mChart.draw(fb, interior);

        // auto outerRadius = interior.minDimension() / 2.0f;
        // auto innerRadius = outerRadius * 0.5;
        // auto radiusSpan = (outerRadius - innerRadius) / 2.0f;
        // auto centerRadius = innerRadius + radiusSpan;

        // int x = interior.center.x;
        // int y = interior.center.y;

        // auto current = mRegister.getChartCurrentIndex();
        // int length = mRegister.getChartSize();
        // float width = M_PI * 2.0f / (float)length;

        // bool isLengthChange = length != mLastLength;
        // mLastLength = length;

        // fb.circle(GRAY1, x, y, innerRadius);
        // fb.circle(GRAY5, x, y, centerRadius);
        // fb.circle(GRAY1, x, y, outerRadius);

        // float lastX = x;
        // float lastY = y;

        // float firstX = x;
        // float firstY = y;

        // float scale = mMaxV > 0 ? 1.0f / mMaxV : 1.0f;
        // mMaxV = 0;

        // float brightness = 0;

        // auto kb = graphics::IKeyboard(world.splitRight(0.333));
        // kb.draw(fb, WHITE, 1);

        // for (int i = 0; i < 128; i++) {
        //   if (isLengthChange) {
        //     mValues[i].hardSet(0);
        //     //continue;
        //   }

        //   if (i >= length) continue;

        //   float theta = width * (float)i;
        //   float amount = mRegister.getChartValue(i);
        //   mMaxV = util::fmax(mMaxV, fabs(amount));

        //   //float rFrom = centerRadius;
        //   float rTo = centerRadius + radiusSpan * amount * scale;
        //   float rMax = util::fmax(rTo, 0.01);
        //   rTo = mValues[i].process(mSlewRate, rTo);

        //   auto brightnessBase = i == current ? GRAY10 : GRAY7;

        //   brightness = rTo / rMax;
        //   brightness = brightness * brightness; // ^2
        //   brightness = brightness * brightness; // ^4
        //   brightness = brightness * brightness; // ^8
        //   brightness = brightnessBase * brightness;

        //   //float fromX = circleX(x, rFrom, theta);
        //   //float fromY = circleY(y, rFrom, theta);

        //   float toX = circleX(x, rTo, theta);
        //   float toY = circleY(y, rTo, theta);

        //   // toX = mXValues[i].process(mSlewRate, toX);
        //   // toY = mYValues[i].process(mSlewRate, toY);

        //   //fb.line(GRAY5, fromX, fromY, toX, toY);
        //   //fb.circle(WHITE, toX, toY, 1);
        //   //fb.pixel(WHITE, toX, toY);

        //   if (i == 0) {
        //     firstX = toX;
        //     firstY = toY;
        //   } else {
        //     fb.line(brightness, lastX, lastY, toX, toY);
        //   }

        //   if (i == current) {
        //     fb.circle(brightnessBase, toX, toY, 1);
        //   }
        //   lastX = toX;
        //   lastY = toY;

        //   //fb.circle(WHITE, toX, toY, 1);
        //   //drawSegment(fb, x, y, centerRadius, centerRadius + radiusSpan * amount, theta);
        //   //drawCirclePixel(fb, WHITE, x, y, centerRadius, theta);
        // }

        // fb.line(brightness, lastX, lastY, firstX, firstY);
      }

      // float circleX(float x, float radius, float theta) {
      //   return (sinf(theta) * radius) + x;
      // }

      // float circleY(float y, float radius, float theta) {
      //   return (cosf(theta) * radius) + y;
      // }

      // void drawSegment(od::FrameBuffer &fb, float x, float y, float rFrom, float rTo, float theta) {
      //   int fromX = circleX(x, rFrom, theta);
      //   int fromY = circleY(y, rFrom, theta);

      //   int toX = circleX(x, rTo, theta);
      //   int toY = circleY(y, rTo, theta);

      //   fb.line(GRAY5, fromX, fromY, toX, toY);
      //   fb.pixel(WHITE, toX, toY);
      // }

      // void drawCirclePixel(od::FrameBuffer &fb, od::Color color, float x, float y, float radius, float theta) {
      //   int toX = circleX(x, radius, theta);
      //   int toY = circleY(y, radius, theta);
      //   fb.pixel(color, toX, toY);
      // }

      //slew::SlewRate mSlewRate { 0.25, GRAPHICS_REFRESH_PERIOD };
      //slew::Slew mValues[128];

      //float mMaxV = 0;
      //float mLastLength = 1;

      //common::HasChartData &mRegister;

      graphics::CircleChart mChart;
  };
}