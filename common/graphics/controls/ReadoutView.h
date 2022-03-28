#pragma once

#include <string>

#include <od/graphics/Graphic.h>
#include <od/objects/Parameter.h>

#include <graphics/composites/Text.h>
#include <graphics/composites/FollowableValue.h>
#include <graphics/composites/FollowableText.h>
#include <graphics/primitives/all.h>

#include <ui/dial/state.h>
#include <ui/DialMap.h>
#include <util/Units.h>

namespace graphics {
  class ReadoutView : public od::Graphic {
    public:
      ReadoutView(od::Parameter *param);

      virtual ~ReadoutView();

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
        mText.setSize(size);
      }

      void setJustifyAlign(graphics::JustifyAlign ja) {
        mJustifyAlign = ja;
      }

      void setCursorOrientation(od::CursorOrientation orientation) {
        mCursorState.orientation = orientation;
      }

      void save() {
        mDialState.save();
      }

      void zero() {
        mDialState.zero();
        setValueInUnits(mDialState.value());
      }

      void restore() {
        mDialState.restore();
        setValueInUnits(mDialState.value());
      }

      void encoder(int change, bool shifted, bool fine) {
        if (mDisplay.hasMoved()) {
          mDialState.set(mDisplay.lastValueInUnits());
        }
        mDialState.move(change, shifted, fine);
        setValueInUnits(mDialState.value());
      }

#ifndef SWIGLUA
      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = Box::extractWorld(*this);
        auto bounds = mText.draw(fb, WHITE, world, mJustifyAlign);
        bounds.pointAtMe(mCursorState);
      }
#endif

    private:
      FollowableValue mDisplay;
      FollowableText mText { mDisplay };
      ui::dial::State mDialState;
      JustifyAlign mJustifyAlign = CENTER_MIDDLE;
  };
}