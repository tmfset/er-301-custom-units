#pragma once

#include <od/graphics/Graphic.h>
#include <math.h>
#include <RegisterLike.h>
#include <util.h>
#include <graphics.h>
#include <slew.h>

namespace lojik {
  class RegisterChart : public od::Graphic {
    public:
      RegisterChart(RegisterLike &regLike, int left, int bottom, int width, int height) :
          od::Graphic(left, bottom, width, height),
          mRegister(regLike) {
        mRegister.attach();
      }

      virtual ~RegisterChart() {
        mRegister.release();
      }
    
    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world    = graphics::Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);
        auto interior = world.inner(2);

        auto current = mRegister.current();
        auto length = mRegister.length();
        auto width = length * mBarWidth + (length - 1) * mSpacing;
        auto hWidth = width / 2.0f;

        auto currentX = barCenterX(current);
        auto chart = graphics::Box::wh(width, mBarHeight * 2.0f)
                       .recenterOn(interior)
                       .offsetY(-mBarHeight * 2.0f);

        auto window = chart.insert(
          chart
            .withWidthFromLeft(interior.width)
            .recenterX(chart.left + currentX)
        );
        auto view = window.recenterOn(interior);
        // chart.clear(fb);
        // chart.outline(fb, WHITE);
        // window.outline(fb, WHITE);

        //view.outline(fb, WHITE);

        auto hBarWidth = mBarWidth / 2.0f;

        for (int i = 0; i < length; i++) {
          auto chartX = chart.left + barCenterX(i);
          auto x = chartX - window.left + view.left;
          auto y = view.centerY;

          auto value = mRegister.value(i);

          if (!view.containsX(x)) continue;

          
          auto bar = graphics::Box::cwr(x, y, mBarWidth, value * mBarHeight);
          //auto bar = graphics::Box::lbwh(x, y, mBarWidth, value * mBarHeight);

          if (i == current) {
            auto cursor = bar.recenterY(y).square(mBarWidth + 2);
            bar.fill(fb, GRAY12);
            cursor.line(fb, WHITE);
          } else {
            bar.fill(fb, GRAY10);
          }
        }
      }

      inline float barLeftX(int i) {
        return mBarWidth * i + mSpacing * i;
      }

      inline float barCenterX(int i) {
        return barLeftX(i) + mBarWidth / 2.0f;
      }

      slew::SlewRate mSlewRate { 0.25, GRAPHICS_REFRESH_PERIOD };
      slew::Slew mValues[128];

      const int mSpacing = 2;
      const int mBarWidth = 3;
      const int mBarHeight = mBarWidth * 3;

      RegisterLike &mRegister;
  };
}