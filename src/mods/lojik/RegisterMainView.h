#pragma once

#include <od/graphics/Graphic.h>
#include <od/graphics/controls/Readout.h>
#include <math.h>
#include <util.h>
#include <graphics.h>
#include <slew.h>
#include <Register2.h>

namespace lojik {
  class RegisterMainView : public od::Graphic {
    public:
      RegisterMainView(Register2 &data, int left, int bottom, int width, int height);

      virtual ~RegisterMainView() { }

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

        //auto interior = world.inner(2);

        auto top = world.splitTop(0.5);
        auto bottom = world.splitBottom(0.5).withHeight(12);

        //mCircleChart.draw(fb, left.scaleHeight(0.75));

        mKeyboard.draw(fb, top);

        mChart.draw(fb, bottom, 3, 2);

        //mScaleList.draw(fb, world, 10);

        auto size = 8;

        auto local = world.atLeftBottom(0, 2);

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
      }

      void setOffsetHighlight(bool v) { mHighlightOffset = v; }
      void setShiftHighlight(bool v)  { mHighlightShift  = v; }
      void setLengthHighlight(bool v) { mHighlightLength = v; }
      void setStrideHighlight(bool v) { mHighlightStride = v; }

      graphics::HChart mChart;
      graphics::CircleChart mCircleChart;
      graphics::HKeyboard mKeyboard;

      graphics::ScaleList mScaleList;

      od::Readout mOffsetReadout;
      od::Readout mShiftReadout;
      od::Readout mLengthReadout;
      od::Readout mStrideReadout;

      bool mHighlightOffset = false;
      bool mHighlightShift  = false;
      bool mHighlightLength = false;
      bool mHighlightStride = false;
  };
}