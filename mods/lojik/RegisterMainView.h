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
#include <graphics/composites/ReadoutList.h>
#include <graphics/composites/GateList.h>
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
        //return mReadoutList.getCursorController(i);
        // switch (i) {
        //   case 0: return &mOffsetReadout;
        //   case 1: return &mShiftReadout;
        //   case 2: return &mLengthReadout;
        //   case 3: return &mStrideReadout;
        // }
        return nullptr;
      }

    private:

      void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        auto world = graphics::Box::extractWorld(*this);
        auto top = world.splitTop(0.45);
        auto bottom = world.splitBottom(0.55);

        top.trace(fb, GRAY7);
        world.traceRight(fb, GRAY7);
        mChart.draw(fb, top.withHeight(21).padX(2), 2, 16);

        auto bottomArea = bottom.padBottom(3);
        auto bottomRight = bottomArea.splitRight(0.5);
        auto bottomLeft = bottomArea.splitLeft(0.5);

        mReadoutList.draw(fb, bottomRight.padLeft(4).padRight(2));
        mGateList.draw(fb, bottomLeft.padLeft(4));

        // auto offsetBox = lower.splitTop(0.5).splitTop(0.5);
        // mOffsetText.draw(fb, WHITE, offsetBox);
        // offsetBox.applyTo(mOffsetReadout, *this);

        // auto shiftBox = lower.splitTop(0.5).splitBottom(0.5);
        // mShiftText.draw(fb, WHITE, shiftBox);
        // shiftBox.applyTo(mShiftReadout, *this);

        // auto lengthBox = lower.splitBottom(0.5).splitTop(0.5);
        // mLengthText.draw(fb, WHITE, lengthBox);
        // lengthBox.applyTo(mLengthReadout, *this);

        // auto strideBox = lower.splitBottom(0.5).splitBottom(0.5);
        // mStrideText.draw(fb, WHITE, strideBox);
        // strideBox.applyTo(mStrideReadout, *this);

        // auto bottom = world.inBottom(12);

        // bottom.outTop(10).applyTo(mLengthReadout);

        // mLengthReadout.draw(fb);

        // mLengthText.draw(fb, WHITE, bottom);
      }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::HKeyboard mKeyboard;

      graphics::ScaleList mScaleList;
      graphics::ReadoutList mReadoutList;
      graphics::GateList mGateList;

  };
}