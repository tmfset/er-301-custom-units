#pragma once

#include "Polygon.h"
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
#include "util.h"

namespace polygon {
  class RoundRobinView : public od::Graphic {
    public:
      RoundRobinView(Polygon &polygon, int left, int bottom, int width, int height) :
        od::Graphic(left, bottom, width, height),
        mPolygon(polygon) {
          mPolygon.attach();

          for (int i = 0; i < mPolygon.getVoiceCount(); i++) {
            auto fade = od::LinearRamp {};
            fade.setLength(GRAPHICS_REFRESH_RATE / 2);
            mFades.push_back(fade);

            mGateCounts.push_back(0);
          }
        }

      virtual ~RoundRobinView() {
        mPolygon.release();
      }

    private:
      void draw(od::FrameBuffer &fb) {
        //fb.text(WHITE, mWorldLeft, mWorldBottom, "TEST");

        for (int i = 0; i < (int)mGateCounts.size(); i++) {
          const auto newCount = mPolygon.getGateCount(i);

          if (newCount > mGateCounts[i]) {
            mFades[i].reset(1.0f, 0.0f);
            mGateCounts[i] = newCount;
          }

          mFades[i].step();
        }

        const float cols = (float)POLYGON_SETS;
        const float rows = (float)4;

        const float width = (float)mWidth;
        const float height = (float)mHeight;
        const float pad = 3.0f;

        const auto totalColPad = pad * (cols + 1);
        const auto totalRowPad = pad * (rows + 1);

        const auto size = util::fmin(
          (width - totalColPad) / cols,
          (height - totalRowPad) / rows
        );

        const auto halfSize = size / 2.0f;

        const auto totalColWidth = size * cols + totalColPad;
        const auto totalRowWidth = size * rows + totalRowPad;

        const auto colMargin = (width - totalColWidth) / 2.0f;
        const auto rowMargin = (height - totalRowWidth) / 2.0f;

        const auto gridLeft = mWorldLeft + colMargin;
        const auto gridBottom = mWorldBottom + rowMargin;

        for (int c = 0; c < cols; c++) {
          const auto left = gridLeft + (c + 1) * pad + c * size;
          const auto x = left + halfSize;

          for (int r = 0; r < rows; r++) {
            const auto index = c * rows + r;
            const auto fadeAmount = mPolygon.getVoiceEnv(index);

            const auto bottom = gridBottom + (r + 1) * pad + r * size;
            const auto y = bottom + halfSize;

            fb.fillCircle(WHITE * fadeAmount, x, y, halfSize);
            fb.circle(GRAY10, x, y, halfSize);
          }
        }
      }

      std::vector<int> mGateCounts;
      std::vector<od::LinearRamp> mFades;
      Polygon &mPolygon;
  };
}