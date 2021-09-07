#pragma once

#include <od/graphics/Graphic.h>
#include "slew.h"

namespace polygon {
  class PagedSubView : public od::Graphic {
    public:
      PagedSubView() : od::Graphic(0, 0, 128, 64) {
        mPageSlew.setRate(0.1, GRAPHICS_REFRESH_PERIOD);
      }
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
         mFrame = (mFrame + 1) % 20;
         const auto pad = 3.0f;
         const auto size = 3.0f;
         const auto center = mHeight / 2.0f;

         mPageSlew.process(mPage);

         for (int i = 0; i < mChildren.size(); i++) {
           auto pageOffset = mPage - i;

           if (pageOffset == 0 && (mFrame % 2) == 0) {
             fb.fillCircle(WHITE, mWidth, center, size);
           } else {
             fb.circle(WHITE, mWidth, center + pageOffset * (size * 2.0f + pad), size);
           }

           mChildren[i]->setPosition(0, (mPageSlew.value() - i) * (mHeight + 1));
         }

         od::Graphic::draw(fb);
       }

       int mPage = 0;
       int mFrame = 0;
       slew::Slew mPageSlew;
  };
}