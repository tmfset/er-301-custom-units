#pragma once

#include <od/graphics/Graphic.h>
#include <od/objects/Followable.h>

#include <graphics/composites/Text.h>
#include <graphics/composites/FollowableValue.h>
#include <graphics/composites/ScaleList.h>

#include <util/Units.h>

#include <ui/dial/state.h>

namespace graphics {
  class ScaleListView : public od::Graphic {
    public:
      ScaleListView(HasScaleBook &data, od::Followable &followable);
      virtual ~ScaleListView();

      void setAttributes(util::Units units, ui::DialMap *map) {
        mValue.setUnits(units);
        mDialState.setMap(map);
      }

      void setValueInUnits(float value) {
        mValue.setValueInUnits(value, true);
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
      FollowableValue mValue;
      ui::dial::State mDialState;
      ScaleList mView;
  };
}