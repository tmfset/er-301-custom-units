#pragma once

#include <graphics/external/PlyElement.h>

namespace graphics {
  class PlyControl : public PlyElement {
    public:
      PlyControl(std::string name) :
        PlyElement() { }

      virtual ~PlyControl() { }

#ifndef SWIGLUA
      void draw(od::FrameBuffer &fb) {
        // ...
      }
#endif

    private:
  };
}
