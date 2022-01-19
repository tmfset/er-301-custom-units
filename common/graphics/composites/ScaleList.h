#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/composites/ListWindow.h>
#include <graphics/composites/Text.h>
#include <graphics/interfaces/all.h>

namespace graphics {
  class ScaleList {
    public:
      inline ScaleList(HasScaleBook &data) : mData(data) {
        mData.attach();
      }

      inline ~ScaleList() {
        mData.release();
      }

      void draw(
        od::FrameBuffer &fb,
        const Box &world,
        int size
      ) {
        auto length = mData.getScaleBookSize();
        auto current = mData.getScaleBookIndex();

        auto window = ListWindow::from(world.vertical(), size, 2)
          .scrollTo(current, length);

        for (int i = 0; i < length; i++) {
          auto y = window.globalCenterFromRight(i);
          if (!window.visible(y)) continue;

          auto name = mData.getScaleName(i);
          //auto size = mData.getScaleSize(i);

          auto wh = world.widthHeight().atY(size);
          auto xy = world.leftCenter().atY(y);

          auto isCurrent = i == current;
          auto box = Box::lbwh(xy, wh);
          //if (isCurrent) Box::cwh(xy, wh).trace(fb, WHITE);
          Text(name, size).draw(fb, WHITE, box, LEFT_MIDDLE, isCurrent);
        }
      }
    private:
      HasScaleBook &mData;
  };
}