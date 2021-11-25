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
      RoundRobinPitchView(Observable &observable, int left, int bottom, int width, int height) :
          od::Graphic(left, bottom, width, height),
          mObservable(observable) {
        mObservable.attach();
      }

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
      int scale(float value, int width) {
        return util::funit(value * mScale) * width;
      }

      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        const float voices = mObservable.voices();

        const graphics::Box world     = graphics::Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);
        const graphics::Box leftPane  = world.divideLeft(0.3f).inner(0, 5);
        const graphics::Box rightPane = world.divideRight(0.7f).inner(0, 0).padRight(6);

        fb.vline(sColor(0), leftPane.center.x, leftPane.bottom, leftPane.top);
        fb.hline(sColor(0), leftPane.center.x - 2, leftPane.center.x + 2, leftPane.bottom);
        fb.hline(sColor(0), leftPane.center.x - 2, leftPane.center.x + 2, leftPane.top);

        od::Parameter* vpoRRDirect = mObservable.vpoDirect(0);
        od::Parameter* vpoRROffset = mObservable.vpoOffset(0);
        const float rrTarget = vpoRRDirect->target() + vpoRROffset->target();
        const float rrActual = vpoRRDirect->value() + vpoRROffset->value();
        const int rrTargetY = util::fhr(leftPane.center.y + scale(rrTarget, leftPane.height / 2.0f));
        const int rrActualY = util::fhr(leftPane.center.y + scale(rrActual, leftPane.height / 2.0f));
        fb.hline(sColor(0), leftPane.center.x - 4, leftPane.center.x + 4, rrActualY);
        fb.box(pColor(0), leftPane.center.x - 3, rrTargetY - 1, leftPane.center.x + 3, rrTargetY + 1);

        if (mCursorSelection == 0) {
          mCursorState.orientation = od::cursorRight;
          mCursorState.x = leftPane.left - 5;
          mCursorState.y = rrTargetY;
        }

        const graphics::Box first = rightPane.divideTop(1.0f / voices);
        for (int i = 0; i < voices; i++) {
          const graphics::Box next = first.offsetY(util::fhr(-first.height) * i);

          const int x0 = util::fhr(next.left);
          const int x1 = util::fhr(next.right);
          const int y  = util::fhr(next.center.y);
          fb.hline(sColor(i + 1), x0, x1, y);
          fb.vline(sColor(i + 1), x0, y - 1, y);
          fb.vline(sColor(i + 1), x1, y, y + 1);

          od::Parameter* vpoDirect = mObservable.vpoDirect(i + 1);
          od::Parameter* vpoOffset = mObservable.vpoOffset(i + 1);
          const float target = vpoDirect->target() + vpoOffset->target();
          const float actual = vpoDirect->value() + vpoOffset->value();

          const int targetX = util::fhr(next.center.x + scale(target, next.width / 2.0f));
          const int actualX = util::fhr(next.center.x + scale(actual, next.width / 2.0f));
          fb.vline(sColor(i + 1), actualX, y - 3, y + 3);
          fb.box(pColor(i + 1), targetX - 1, y - 2, targetX + 1, y + 2);

          const int indX = next.right + 3;
          const int indY = next.center.y;

          if (mObservable.isVoiceArmed(i)) {
            fb.pixel(pColor(i + 1), indX - 1, indY);
            fb.pixel(pColor(i + 1), indX + 1, indY);
            fb.pixel(pColor(i + 1), indX, indY - 1);
            fb.pixel(pColor(i + 1), indX, indY + 1);
          }

          if (mObservable.isVoiceNext(i)) {
            fb.pixel(pColor(i + 1), indX, indY);
          }

          if (mCursorSelection == i + 1) {
            mCursorState.orientation = od::cursorRight;
            mCursorState.x = next.left - 10;
            mCursorState.y = next.center.y;
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