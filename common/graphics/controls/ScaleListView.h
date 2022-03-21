#pragma once

#include <od/graphics/Graphic.h>
#include <od/objects/Parameter.h>

#include <graphics/composites/Text.h>
#include <graphics/composites/ScaleList.h>

#include <ui/dial/state.h>

namespace graphics {
  class ScaleListView : public od::Graphic {
    public:
      ScaleListView(HasScaleBook &data, od::Parameter *param);
      virtual ~ScaleListView();

      void setAttributes(util::Units units, ui::DialMap *map) {
        mDisplay.setUnits(units);
        mDialState.setMap(map);
      }

      void setValueInUnits(float value) {
        mDisplay.setValueInUnits(value, true);
      }

      void encoder(int change, bool shifted, bool fine) {
        // if (mDisplay.hasMoved()) {
        //   mDialState.set(mDisplay.lastValueInUnits());
        // }
        mDialState.move(change, shifted, fine);
        setValueInUnits(mDialState.value());
      }

#ifndef SWIGLUA
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = Box::extractWorld(*this);
        auto bounds = mView.draw(fb, world, 5);
        bounds.pointAtMe(mCursorState);
      }
#endif

    private:
      ParameterDisplay mDisplay;
      ui::dial::State mDialState;
      ScaleList mView;
  };
}