#pragma once

#include <od/graphics/Graphic.h>

namespace graphics {
  class MainControl : public od::Graphic {
    public:
      MainControl(int plyWidth) :
        Graphic(0, 0, plyWidth * SECTION_PLY, SCREEN_HEIGHT),
        mPlyWidth(plyWidth) { }

      virtual ~MainControl() { }

      int plyWidth() { return mPlyWidth; }

    private:
      int mPlyWidth;
  };
}