#pragma once

#include <hal/constants.h>
#include <od/graphics/Graphic.h>
#include "slew.h"

namespace common {
  class PagedGraphic : public od::Graphic {
    public:
      PagedGraphic(int left, int bottom, int width, int height) : od::Graphic(left, bottom, width, height) { }
      virtual ~PagedGraphic() {}

      void switchPage() {
        mPage = (mPage + 1) % mChildren.size();
      }

      void setPage(int page) {
        mPage = page % mChildren.size();
      }

      int currentPage() {
        return mPage;
      }

    private:
      void draw(od::FrameBuffer &fb) {
        advanceFrame();
        auto slew = mPageSlew.process(mPageSlewRate, mPage);

        const int pageCount   = mChildren.size();
        const int isMultiPage = pageCount > 1;
        for (int i = 0; i < pageCount; i++) {
          if (isMultiPage) drawPageIndicator(fb, WHITE, i);
          mChildren[i]->setPosition(0, (slew - i) * (mHeight + 1));
        }

        od::Graphic::draw(fb);
      }

      inline void drawPageIndicator(od::FrameBuffer &fb, int color, int page) const {
        const auto pad    = 3.0f;
        const auto size   = 3.0f;
        const auto center = mHeight / 2.0f;

        const auto offset    = mPage - page;
        const auto isCurrent = offset == 0;

        if (isCurrent && isFillFrame()) {
          fb.fillCircle(color, mWidth, center, size);
        } else {
          fb.circle(color, mWidth, center + offset * (size * 2.0f + pad), size);
        }
      }

      inline void advanceFrame() {
        mFrame = (mFrame + 1) % (int)GRAPHICS_REFRESH_RATE;
      }

      inline bool isFillFrame() const {
        //return mFrame > 1;
        return true;
      }

      int mPage = 0;
      int mFrame = 0;
      slew::SlewRate mPageSlewRate = slew::SlewRate::fromRate(0.1, GRAPHICS_REFRESH_PERIOD);
      slew::Slew mPageSlew;
  };
}