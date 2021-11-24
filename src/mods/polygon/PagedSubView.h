#pragma once

#include <od/graphics/Graphic.h>
#include "slew.h"

namespace polygon {
  class PagedSubView : public od::Graphic {
    public:
      PagedSubView() : od::Graphic(0, 0, 128, 64) { }
      virtual ~PagedSubView() {}

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

        if (isCurrent && isEvenFrame()) {
          fb.fillCircle(color, mWidth, center, size);
        } else {
          fb.circle(color, mWidth, center + offset * (size * 2.0f + pad), size);
        }
      }

      inline void advanceFrame() {
        mFrame = (mFrame + 1) % 20;
      }

      inline bool isEvenFrame() const {
        return (mFrame % 2) == 0;
      }

      int mPage = 0;
      int mFrame = 0;
      slew::SlewRate mPageSlewRate { 0.1, GRAPHICS_REFRESH_PERIOD };
      slew::Slew mPageSlew;
  };
}