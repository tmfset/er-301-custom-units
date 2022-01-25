#pragma once

#include <od/graphics/Graphic.h>
#include <od/objects/Parameter.h>

#include <graphics/composites/ScaleList.h>

namespace graphics {
  class ScaleListView : public od::Graphic {
    public:
      ScaleListView(HasScaleBook &data);
      virtual ~ScaleListView();

#ifndef SWIGLUA
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = Box::extractWorld(*this);
        mView.draw(fb, world, 8);
      }
#endif

    private:
      ScaleList mView;
  };
}