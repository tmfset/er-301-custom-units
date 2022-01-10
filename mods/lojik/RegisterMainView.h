#pragma once

#include <od/graphics/Graphic.h>
#include <od/graphics/controls/Readout.h>
#include <math.h>
#include <util/math.h>
#include <graphics/primitives/all.h>
#include <graphics/composites/BarChart.h>
#include <graphics/composites/CircleChart.h>
#include <graphics/composites/Keyboard.h>
#include <graphics/composites/ScaleList.h>
#include <dsp/slew.h>
#include <Register2.h>

namespace lojik {
  class RegisterMainView : public od::Graphic {
    public:
      RegisterMainView(Register2 &data, int left, int bottom, int width, int height);

      virtual ~RegisterMainView() { }

      inline od::Readout* getCursorController(int i) {
        switch (i) {
          case 0: return &mOffsetReadout;
          case 1: return &mShiftReadout;
          case 2: return &mLengthReadout;
          case 3: return &mStrideReadout;
        }
        return nullptr;
      }

    private:
      inline void configureReadout(
        od::Readout &readout,
        od::Parameter *parameter
      ) {
        own(readout);
        addChild(&readout);
        readout.setParameter(parameter);
        readout.setPrecision(0);
      }

      inline void configureReadouts() {
        mOffsetReadout.setJustification(od::justifyRight);
        mOffsetReadout.mCursorState.orientation = od::cursorRight;

        mShiftReadout.setJustification(od::justifyLeft);
        mShiftReadout.mCursorState.orientation = od::cursorLeft;

        mLengthReadout.setJustification(od::justifyRight);
        mLengthReadout.mCursorState.orientation = od::cursorRight;

        mStrideReadout.setJustification(od::justifyLeft);
        mStrideReadout.mCursorState.orientation = od::cursorLeft;
      }

      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::extractWorld(*this);

        auto top = world.splitTop(0.333);
        auto bottom = world.splitBottom(0.666);

        //mCircleChart.draw(fb, left.scaleHeight(0.75));

        mChart.draw(fb, bottom.padY(10), 4, 2);

        mKeyboard.draw(fb, top, 0.85);

        //mScaleList.draw(fb, world, 10);

        auto size = 8;

        auto local = world.withHeight(8).atLeftBottom(0, 2);

        auto left = local.splitLeft(0.5);
        left.splitLeftPad(0.5, 2).applyTo(mOffsetReadout);
        left.splitRightPad(0.5, 2).applyTo(mShiftReadout);

        auto right = local.splitRight(0.5);
        right.splitLeftPad(0.5, 2).applyTo(mLengthReadout);
        right.splitRightPad(0.5, 2).applyTo(mStrideReadout);
      }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::HKeyboard mKeyboard;

      graphics::ScaleList mScaleList;

      od::Readout mOffsetReadout;
      od::Readout mShiftReadout;
      od::Readout mLengthReadout;
      od::Readout mStrideReadout;
  };
}