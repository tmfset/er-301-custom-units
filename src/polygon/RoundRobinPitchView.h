#pragma once

#include "Observable.h"
#include <od/ui/DialMap.h>
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
#include <od/extras/Conversions.h>
#include "util.h"
#include "Box.h"

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
        float min = fromCents(map.min());
        float max = fromCents(map.max());
        float size = max - min;
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

        const Box world     = Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);
        const Box leftPane  = world.divideLeft(0.3f).inner(0, 5);
        const Box rightPane = world.divideRight(0.7f).inner(0, 0).padRight(4);

        fb.vline(GRAY7, leftPane.centerX, leftPane.bottom, leftPane.top);
        fb.hline(GRAY7, leftPane.centerX - 2, leftPane.centerX + 2, leftPane.bottom);
        fb.hline(GRAY7, leftPane.centerX - 2, leftPane.centerX + 2, leftPane.top);

        od::Parameter* vpoRoundRobin = mObservable.vpoRoundRobin();
        const float rrTarget = vpoRoundRobin->target();
        const float rrActual = vpoRoundRobin->value();
        const int rrTargetY = util::fhr(leftPane.centerY + scale(rrTarget, leftPane.height / 2.0f));
        const int rrActualY = util::fhr(leftPane.centerY + scale(rrActual, leftPane.height / 2.0f));
        fb.hline(GRAY7, leftPane.centerX - 4, leftPane.centerX + 4, rrActualY);
        fb.box(WHITE, leftPane.centerX - 3, rrTargetY - 1, leftPane.centerX + 3, rrTargetY + 1);

        if (mCursorSelection == 0) {
          mCursorState.orientation = od::cursorRight;
          mCursorState.x = leftPane.left - 5;
          mCursorState.y = rrTargetY;
        }

        const Box first = rightPane.divideTop(1.0f / voices);
        for (int i = 0; i < voices; i++) {
          const Box next = first.offsetY(util::fhr(-first.height) * i);

          const int x0 = util::fhr(next.left);
          const int x1 = util::fhr(next.right);
          const int y  = util::fhr(next.centerY);
          fb.hline(GRAY7, x0, x1, y);
          fb.vline(GRAY7, x0, y - 1, y);
          fb.vline(GRAY7, x1, y, y + 1);

          od::Parameter* vpoDirect = mObservable.vpoDirect(i);
          od::Parameter* vpoOffset = mObservable.vpoOffset(i);
          const float target = vpoDirect->target() + vpoOffset->target();
          const float actual = vpoDirect->value() + vpoOffset->value();

          const int targetX = util::fhr(next.centerX + scale(target, next.width / 2.0f));
          const int actualX = util::fhr(next.centerX + scale(actual, next.width / 2.0f));
          fb.vline(GRAY7, actualX, y - 3, y + 3);
          fb.box(WHITE, targetX - 1, y - 2, targetX + 1, y + 2);

          if (mObservable.isVoiceArmed(i)) {
            fb.fillCircle(WHITE, next.right + 3, next.centerY, 1);
          }

          if (mCursorSelection == i + 1) {
            mCursorState.orientation = od::cursorRight;
            mCursorState.x = next.left - 10;
            mCursorState.y = next.centerY;
          }
        }
      }

      float mScale = 0;
      int mCursorSelection = 0;

      Observable &mObservable;
  };
}