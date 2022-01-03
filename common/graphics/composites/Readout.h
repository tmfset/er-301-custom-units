#pragma once

#include <od/objects/Parameter.h>

#include <graphics/composites/Text.h>
#include <graphics/primitives/all.h>

namespace graphics {
  class Readout {
    public:
      inline Readout(od::Parameter &parameter) :
          mParameter(parameter) {
        mParameter.attach();
      }

      inline ~Readout() {
        mParameter.release();
      }

      void draw(
        od::FrameBuffer &fb,
        od::Color color,
        const v2d &v,
        JustifyAlign ja,
        bool outline
      ) const {
        mText.draw(fb, color, v, ja, outline);
      }

      void update(int size) {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), "%d", (int)mParameter.value());
        std::string v = tmp;
        mText.update(v, size);
      }

    private:
      od::Parameter &mParameter;
      Text mText;
  };
}