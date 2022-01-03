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

      void setCursorSelection(int i) {
        mCursorSelection = i;
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

        mOffsetReadout.setJustification(od::justifyRight);
        mShiftReadout.setJustification(od::justifyLeft);
        mLengthReadout.setJustification(od::justifyRight);
        mStrideReadout.setJustification(od::justifyLeft);

        // if (mCursorSelection == 0) mCursorState.show = false;
        // else switch (mCursorSelection) {
        //   case 1: break;
        //   case 2: break;
        //   case 3: break;
        //   case 4: break;
        // }

        // if (mCursorSelection == 0) {
        //   mCursorState.orientation = od::cursorDown;
        //   mCursorState.x = world.topCenter().x();
        //   mCursorState.y = world.topCenter().y();
        // } else {
        //   mCursorState.orientation = od::cursorDown;
        //   mCursorState.x = 
        // }

        if (highlightOffset()) graphics::Box::extractWorld(mOffsetReadout).trace(fb, WHITE);
        if (highlightShift())  graphics::Box::extractWorld(mShiftReadout).trace(fb, WHITE);
        if (highlighLength())  graphics::Box::extractWorld(mLengthReadout).trace(fb, WHITE);
        if (highlightStride()) graphics::Box::extractWorld(mStrideReadout).trace(fb, WHITE);
      }

      inline bool highlightOffset() const { return mCursorSelection == 1; }
      inline bool highlightShift()  const { return mCursorSelection == 2; }
      inline bool highlighLength()  const { return mCursorSelection == 3; }
      inline bool highlightStride() const { return mCursorSelection == 4; }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::HKeyboard mKeyboard;

      graphics::ScaleList mScaleList;

      od::Readout mOffsetReadout;
      od::Readout mShiftReadout;
      od::Readout mLengthReadout;
      od::Readout mStrideReadout;

      int mCursorSelection = 0;
  };
}