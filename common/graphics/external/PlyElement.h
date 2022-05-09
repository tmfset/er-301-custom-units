#pragma once

#include <od/graphics/Graphic.h>

namespace graphics {
  class PlyElement : public od::Graphic {
    public:
      PlyElement() :
        PlyElement(1) { }

      PlyElement(int ply) :
        PlyElement(ply, 1.0) { }

      PlyElement(int ply, float height) :
        od::Graphic(0, 0, ply * SECTION_PLY, SCREEN_HEIGHT) { }

      virtual ~PlyElement() { }
  };
}
