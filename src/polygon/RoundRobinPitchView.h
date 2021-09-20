#pragma once

#include "Observable.h"
#include <od/ui/DialMap.h>
#include <od/graphics/Graphic.h>
#include <od/extras/LinearRamp.h>
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
        float min = map.min();
        float max = map.max();
        float size = max - min;
        mScale = 1.0f;// / size;
      }

    private:
      void draw(od::FrameBuffer &fb) {
        const float voices = mObservable.voices();

        const Box world     = Box::lbwh(mWorldLeft, mWorldBottom, mWidth, mHeight);
        const Box leftPane  = world.divideLeft(0.4f).inner(0, 5);
        const Box rightPane = world.divideRight(0.6f).inner(0, 0);

        fb.vline(GRAY7, leftPane.centerX, leftPane.bottom, leftPane.top);
        fb.hline(GRAY7, leftPane.centerX - 2, leftPane.centerX + 2, leftPane.bottom);
        fb.hline(GRAY7, leftPane.centerX - 2, leftPane.centerX + 2, leftPane.top);

        od::Parameter* vpoRoundRobin = mObservable.vpoRoundRobin();
        const float rrTarget = vpoRoundRobin->value();
        const float rrCenter = leftPane.centerY;
        const int rrValue = util::fhr(rrCenter + rrTarget * leftPane.height * mScale);
        fb.box(WHITE, leftPane.centerX - 3, rrValue - 1, leftPane.centerX + 3, rrValue + 1);

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
          const float target = vpoDirect->value() + vpoOffset->value();

          const float center = next.centerX;
          const int value = util::fhr(center + target * next.width * mScale);
          fb.box(WHITE, value - 1, y - 2, value + 1, y + 2);
        }
      }

      float mScale = 0;

      Observable &mObservable;
  };
}