#pragma once

#include <od/graphics/Graphic.h>

namespace graphics {
  class Control : public od::Graphic {
    public:
      Control() :
        od::Graphic(0, 0, SECTION_PLY, SCREEN_HEIGHT) { }

      virtual ~Control() { }

      virtual void encoder(int change, bool shifted, bool fine) = 0;
  };
}