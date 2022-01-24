#pragma once

#include <string>

#include <od/graphics/Graphic.h>
#include <od/objects/Parameter.h>

#include <graphics/composites/Text.h>
#include <graphics/primitives/all.h>

#include <ui/dial/state.h>
#include <ui/DialMap.h>
#include <util/Units.h>

namespace graphics {
  class ParameterReadout : public od::Graphic {
    public:
      ParameterReadout(od::Parameter *param);

      virtual ~ParameterReadout();

      void setAttributes(util::Units units, ui::DialMap *map) {
        mDisplay.setUnits(units);
        mDialState.setMap(map);
      }

      void setPrecision(int precision) {
        mText.setPrecision(precision);
      }

      float getValueInUnits() {
        return mDisplay.lastValueInUnits();
      }

      void setValueInUnits(float value) {
        mDisplay.setValueInUnits(value, true);
      }

      void setFontSize(int size) {
        mText.setFontSize(size);
      }

      void setJustifyAlign(graphics::JustifyAlign ja) {
        mText.setJustifyAlign(ja);
      }

      void setCursorOrientation(od::CursorOrientation orientation) {
        mCursorState.orientation = orientation;
      }

      void save()    { mDialState.save(); }
      void zero()    { mDialState.zero(); }
      void restore() { mDialState.restore(); }

      void encoder(int change, bool shifted, bool fine) {
        mDialState.move(change, shifted, fine);
        setValueInUnits(mDialState.value());
      }

#ifndef SWIGLUA
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = Box::extractWorld(*this);
        auto bounds = mText.draw(fb, WHITE, world);
        bounds.pointAtMe(mCursorState);
      }
#endif

    private:
      ParameterDisplay mDisplay;
      ParameterText mText { mDisplay };
      ui::dial::State mDialState;
  };
}