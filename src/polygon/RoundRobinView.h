#pragma once

#include "Observable.h"
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
#include "util.h"
#include "Box.h"

using namespace polygon;

namespace polygon {
  class RoundRobinView : public od::Graphic {
    public:
      RoundRobinView(Observable &observable, int left, int bottom, int width, int height) :
          od::Graphic(left, bottom, width, height),
          mObservable(observable) {
        mObservable.attach();
      }

      virtual ~RoundRobinView() {
        mObservable.release();
      }

    private:
      void draw(od::FrameBuffer &fb) {
        const float cols = (float)mObservable.groups();
        const float rows = (float)4;

        const float pad = 1.5;

        const Box world  = Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);

        const Box corner = world.divideLeft(1.0f / cols).divideTop(1.0f / rows);
        const Box inner = world.inner(
          util::fmax((world.width - corner.minDimension() * cols) / 2.0f, pad),
          util::fmax((world.height - corner.minDimension() * rows) / 2.0f, pad)
        );

        const Box column = inner.divideLeft(1.0f / cols);
        const Box row    = inner.divideTop(1.0f / rows);

        for (int c = 0; c < cols; c++) {
          const Box cBox = column.offsetX(util::fhr(column.width) * c);
          for (int r = 0; r < rows; r++) {
            const Box rBox = row.offsetY(util::fhr(-row.height) * r);

            const Box box = cBox.intersect(rBox).inner(pad);
            const int radius = util::fhr(box.minDimension() * 0.5);

            const int index = c * rows + r;
            const float fillColor = mObservable.envLevel(index);

            const int x = util::fhr(box.centerX);
            const int y = util::fhr(box.centerY);

            fb.fillCircle(WHITE * fillColor, x, y, radius);
            fb.circle(GRAY10, x, y, radius);
          }
        }
      }

      Observable &mObservable;
  };
}