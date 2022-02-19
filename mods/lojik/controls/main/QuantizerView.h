#pragma once

#include <od/graphics/Graphic.h>
#include <graphics/interfaces/all.h>
#include <graphics/controls/MainControl.h>
#include <graphics/composites/Keyboard.h>
#include <graphics/composites/ScaleList.h>

namespace lojik {
  class QuantizerView : public graphics::MainControl {
    public:
      QuantizerView(graphics::Quantizer &data);
      virtual ~QuantizerView() { }

    private:
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::extractWorld(*this);
        mKeyboard.draw(fb, world.splitLeft(0.5), 0.85);
        mScaleList.draw(fb, world.splitRight(0.5).padLeft(5).padY(10), 5);
      }

      graphics::IKeyboard mKeyboard;
      graphics::ScaleList mScaleList;
  };
}