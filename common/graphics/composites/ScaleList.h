#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/composites/ListWindow.h>
#include <graphics/composites/Text.h>
#include <graphics/interfaces/all.h>

#include <dsp/slew.h>

namespace graphics {
  class ScaleList {
    public:
      inline ScaleList(HasScaleBook &data) : mData(data) {
        mData.attach();
      }

      inline ~ScaleList() {
        mData.release();
      }

      Box draw(
        od::FrameBuffer &fb,
        const Box &world,
        int size
      ) {
        auto length = mData.getScaleBookSize();
        auto current = mData.getScaleBookIndex();

        auto slewCurrent = mIndexSlew.process(mIndexSlewRate, current);

        auto window = ListWindow::from(world.vertical(), size, 1)
          .scrollTo(slewCurrent, length);

        auto currentLb = world.leftBottom();
        auto currentRt = world.rightTop();

        for (int i = 0; i < length; i++) {
          auto y = window.globalCenterFromRight(i);
          if (!window.visible(y)) continue;

          auto name = mData.getScaleName(i);
          auto scaleLength = mData.getScaleSize(i);

          auto wh = world.widthHeight().atY(size);
          auto xy = world.leftCenter().atY(y);

          auto color = GRAY10;

          auto isCurrent = i == current;
          auto box = Box::lbwh(xy, wh);
          if (isCurrent) {
            currentLb = box.leftBottom();
            currentRt = box.rightTop();
            color = WHITE;
          }

          auto text = Text(name, size);
          text.setJustifyAlign(LEFT_MIDDLE);
          //text.setOutline(isCurrent);
          text.draw(fb, color, box);
        }

        return Box::lbrt(currentLb, currentRt);
      }

    private:
      slew::SlewRate mIndexSlewRate = slew::SlewRate::fromRate(0.1, GRAPHICS_REFRESH_PERIOD);
      slew::Slew mIndexSlew;
      HasScaleBook &mData;
  };
}