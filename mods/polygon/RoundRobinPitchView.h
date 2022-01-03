#pragma once

#include "Observable.h"
#include <od/ui/DialMap.h>
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
#include <od/extras/Conversions.h>
#include "util.h"
#include "graphics.h"

using namespace polygon;

namespace polygon {
  class RoundRobinPitchView : public od::Graphic {
    public:
      RoundRobinPitchView(Observable &observable, int left, int bottom, int width, int height);
      // RoundRobinPitchView(Observable &observable, int left, int bottom, int width, int height) :
      //     od::Graphic(left, bottom, width, height),
      //     mObservable(observable) {
      //   mObservable.attach();
      // }

      virtual ~RoundRobinPitchView() {
        mObservable.release();
      }

      void setScale(od::DialMap &map) {
        //float min = fromCents(map.min());
        float max = fromCents(map.max());
        mScale = 1.0f / max;
      }

      void setCursorSelection(int value) {
        mCursorSelection = value;
      }

    private:
      float scale(float value) {
        return util::funit(value * mScale);
      }

      float target(od::Parameter* direct, od::Parameter* offset) {
        return direct->target() + offset->target();
      }

      float actual(od::Parameter* direct, od::Parameter* offset) {
        return direct->value() + offset->value();
      }

      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        const float voices = mObservable.voices();

        auto world = graphics::Box::extractWorld(*this);
        auto leftPane  = world.splitLeft(0.3f).pad(2, 5);
        auto rightPane = world.splitRight(0.7f).padRight(6);

        auto pFader = graphics::IFader::box(leftPane);
        pFader.draw(fb, sColor(0));

        od::Parameter* vpoRRDirect = mObservable.vpoDirect(0);
        od::Parameter* vpoRROffset = mObservable.vpoOffset(0);
        pFader.drawActual(fb, sColor(0), scale(actual(vpoRRDirect, vpoRROffset)));
        auto rrTargetY = pFader.drawTarget(fb, pColor(0), scale(target(vpoRRDirect, vpoRROffset)));

        if (mCursorSelection == 0) {
          mCursorState.orientation = od::cursorRight;
          mCursorState.x = leftPane.left() - 5;
          mCursorState.y = rrTargetY;
        }

        auto grid = graphics::Grid::createRect(rightPane, 1, voices, 0);
        for (int i = 0; i < voices; i++) {
          auto cell = grid.cell(0, i);
          auto vFader = graphics::HFader::box(cell);

          auto primaryColor = pColor(i + 1);
          auto secondaryColor = sColor(i + 1);
          vFader.draw(fb, secondaryColor);

          od::Parameter* vpoDirect = mObservable.vpoDirect(i + 1);
          od::Parameter* vpoOffset = mObservable.vpoOffset(i + 1);
          vFader.drawActual(fb, secondaryColor, scale(actual(vpoDirect, vpoOffset)));
          vFader.drawTarget(fb, primaryColor, scale(target(vpoDirect, vpoOffset)));

          auto indicator = graphics::Point(cell.rightCenter() + v2d::of(3, 0));

          if (mObservable.isVoiceArmed(i)) {
            indicator.diamond(fb, primaryColor);
          }

          if (mObservable.isVoiceNext(i)) {
            indicator.dot(fb, primaryColor);
          }

          if (mCursorSelection == i + 1) {
            mCursorState.orientation = od::cursorRight;
            mCursorState.x = vFader.left - 10;
            mCursorState.y = vFader.y;
          }
        }
      }

      bool isFaded(int i) {
        if (mCursorSelection == 0) return false;
        return mCursorSelection != i;
      }

      int pColor(int i) {
        return isFaded(i) ? mPrimaryColor - mColorFade : mPrimaryColor;
      }

      int sColor(int i) {
        return isFaded(i) ? mSecondaryColor - mColorFade : mSecondaryColor;
      }

      int mPrimaryColor = WHITE;
      int mSecondaryColor = GRAY7;
      int mColorFade = 5;

      float mScale = 0;
      int mCursorSelection = 0;

      Observable &mObservable;
  };
}