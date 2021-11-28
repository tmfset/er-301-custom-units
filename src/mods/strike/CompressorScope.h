#pragma once

#include <od/objects/measurement/FifoProbe.h>
#include <od/graphics/Graphic.h>
#include <od/graphics/text/Label.h>
#include <od/extras/FastEWMA.h>
#include <od/AudioThread.h>
#include <graphics.h>
#include <SimpleScope.h>
#include <CPR.h>

namespace strike {
  class CompressorScope : public od::Graphic {
    public:
      CompressorScope(int left, int bottom, int width, int height);

      virtual ~CompressorScope() {
        clearObjects();
      }

#ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb) {
        od::Graphic::draw(fb);

        auto world = graphics::Box::lbwh_raw(mWorldLeft, mWorldBottom, mWidth, mHeight);
        auto frame = world.inner(10);

        drawThreshold(fb, frame, GRAY10);

        fb.vline(GRAY8, frame.left, frame.bottom, frame.top);

        drawGuidlines(fb, frame, GRAY10);
      }
#endif

      void watch(CPR *pCompressor) {
        clearObjects();
        mpThreshold = pCompressor->getParameter("Threshold");
        mpThreshold->attach();

        mScopeExcite.watch(pCompressor->getOutput("Excite"));
        mScopeReduction.watch(pCompressor->getOutput("Reduction"));
      }

    protected:
      SimpleScope mScopeExcite;
      SimpleScope mScopeReduction;
      od::Parameter *mpThreshold = 0;

      void drawThreshold(od::FrameBuffer &fb, const graphics::Box &world, od::Color color) {
        auto threshold = world.scaleHeight(mpThreshold->value());
        threshold.lineTopIn(fb, color, 2);
        threshold.lineBottomIn(fb, color, 2);
      }

      void drawGuidlines(od::FrameBuffer &fb, const graphics::Box &world, int color) {
        auto height  = world.height;
        auto quarter = height / 4.0f;
        auto eigth   = height / 8.0f;

        auto center = util::fhr(world.center.y);
        auto left   = world.left + 1;
        auto right  = world.right - 1;

        auto threeEigths = util::fhr(quarter + eigth);
        auto twoEigths = util::fhr(quarter);
        auto oneEigth = util::fhr(eigth);
        fb.hline(color, left,  left  + 1, center + threeEigths);
        fb.hline(color, right, right - 1, center + threeEigths);
        fb.hline(color, left,  left  + 1, center - threeEigths);
        fb.hline(color, right, right - 1, center - threeEigths);

        fb.hline(color, left,  left  + 3, center + twoEigths);
        fb.hline(color, right, right - 3, center + twoEigths);
        fb.hline(color, left,  left  + 3, center - twoEigths);
        fb.hline(color, right, right - 3, center - twoEigths);

        fb.hline(color, left,  left  + 1, center + oneEigth);
        fb.hline(color, right, right - 1, center + oneEigth);
        fb.hline(color, left,  left  + 1, center - oneEigth);
        fb.hline(color, right, right - 1, center - oneEigth);
      }

      void clearObjects() {
        if (mpThreshold) {
          mpThreshold->release();
          mpThreshold = 0;
        }
      }
  };

}
