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

        const int cols = POLYGON_SETS;
        const int rows = 4;

        const auto width = (float)mWidth;
        const auto height = (float)mHeight;
        const auto pad = 3.0f;

        const auto size = util::fmin(
          (width - pad * (cols + 1)) / (float)cols,
          (height - pad * (rows + 1)) / (float)rows
        );

        const auto widthCenteringOffset = (width - size * cols - pad * (cols + 1)) / 2.0f;
        const auto heightCenteringOffset = (height - size * rows - pad * (rows + 1)) / 2.0f;

        const auto gridLeft = mWorldLeft + widthCenteringOffset;
        const auto gridBottom = mWorldBottom + heightCenteringOffset;

        for (int c = 0; c < cols; c++) {
          const auto left = gridLeft + (c + 1) * pad + c * size;
          //const auto right = left + size;
          const auto x = left + size / 2.0f;

          for (int r = 0; r < rows; r++) {
            const auto index = c * rows + r;
            const auto fadeAmount = mPolygon.getVoiceEnv(index);
            //const auto fadeAmount = mFades[index].mValue;

            const auto bottom = gridBottom + (r + 1) * pad + r * size;
            //const auto top = bottom + size;
            const auto y = bottom + size / 2.0f;

            fb.fillCircle(WHITE * fadeAmount, x, y, size / 2.0f);
            fb.circle(WHITE, x, y, size / 2.0f);
            
            
            //fb.box(WHITE, left, bottom, right, top);
          }
        }
      }

      std::vector<int> mGateCounts;
      std::vector<od::LinearRamp> mFades;
      Polygon &mPolygon;
  };
}