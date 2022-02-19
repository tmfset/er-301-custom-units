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
#include <graphics/controls/MainControl.h>
#include <graphics/controls/ReadoutView.h>
#include <dsp/slew.h>
#include <Register2.h>

namespace lojik {
  class RegisterMainView : public graphics::MainControl {
    public:
      RegisterMainView(Register2 &data);

      virtual ~RegisterMainView() { }

      inline graphics::ReadoutView* getCursorController(int i) {
        switch (i) {
          case 0: return &mOffsetReadout;
          case 1: return &mShiftReadout;
          case 2: return &mLengthReadout;
          case 3: return &mStrideReadout;
        }
        return nullptr;
      }

    private:
      inline void configureReadout(graphics::ReadoutView &readout) {
        own(readout);
        //addChild(&readout);
        readout.setPrecision(0);
        readout.setFontSize(8);
      }

      inline void configureReadouts() {
        mOffsetReadout.setJustifyAlign(RIGHT_MIDDLE);
        mOffsetReadout.setCursorOrientation(od::cursorRight);

        mShiftReadout.setJustifyAlign(LEFT_MIDDLE);
        mShiftReadout.setCursorOrientation(od::cursorLeft);

        mLengthReadout.setJustifyAlign(CENTER_MIDDLE);
        mLengthReadout.setCursorOrientation(od::cursorRight);

        mStrideReadout.setJustifyAlign(LEFT_MIDDLE);
        mStrideReadout.setCursorOrientation(od::cursorLeft);
      }

      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::extractWorld(*this);
        mChart.draw(fb, world.splitTop(0.8).withHeight(21).padX(3), 2);

        auto bottom = world.inBottom(12);

        bottom.outTop(10).applyTo(mLengthReadout);

        mLengthReadout.draw(fb);

        mLengthText.draw(fb, WHITE, bottom);
      }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::IKeyboard mKeyboard;

      graphics::ScaleList mScaleList;

      graphics::ReadoutView mOffsetReadout;
      graphics::ReadoutView mShiftReadout;
      graphics::ReadoutView mLengthReadout;
      graphics::ReadoutView mStrideReadout;

      graphics::Text mLengthText { "OSLS", 8 };
  };
}