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
      ParameterReadout(int l, int b, int w, int h);
      ~ParameterReadout();

      void setParameter(od::Parameter *param) {
        mDisplay.setParameter(param);
      }

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
        mDisplay.setValueInUnits(value, false);
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
        mText.draw(fb, WHITE, world);
        // Set mCursorState!
      }
#endif

    private:
      ParameterDisplay mDisplay;
      ParameterText mText { mDisplay };
      ui::dial::State mDialState;
  };
}