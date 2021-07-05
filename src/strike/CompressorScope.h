#pragma once

#include <od/objects/measurement/FifoProbe.h>
#include <od/graphics/Graphic.h>
#include <od/graphics/text/Label.h>
#include <od/extras/FastEWMA.h>
#include <od/AudioThread.h>
#include <SimpleScope.h>
#include <CPR.h>

namespace strike {
  class CompressorScope : public od::Graphic {
    public:
      CompressorScope(
        int left,
        int bottom,
        int width,
        int height
      ) : od::Graphic(left, bottom, width, height),
          mScopeExcite(left, bottom, width, height, WHITE, true),
          mScopeReduction(left, bottom, width, height, WHITE, false) {
        own(mScopeExcite);
        addChild(&mScopeExcite);

        own(mScopeReduction);
        addChild(&mScopeReduction);
      }

      virtual ~CompressorScope() {
        clearObjects();
      }

#ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb) {
        od::Graphic::draw(fb);

        drawThreshold(fb, GRAY10);

        fb.vline(GRAY8, mWorldLeft, mWorldBottom + 1, mWorldBottom + mHeight - 2);

        drawGuidlines(fb, GRAY10);
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

      void drawThreshold(od::FrameBuffer &fb, int color) {
        auto height = mHeight - 1;
        auto halfHeight = height / 2.0f;
        auto center = mWorldBottom + halfHeight;
        auto threshold = mpThreshold->value() * halfHeight;
        fb.hline(color, mWorldLeft, mWorldLeft + mWidth - 1, center + threshold, 2);
        fb.hline(color, mWorldLeft, mWorldLeft + mWidth - 1, center - threshold, 2);
      }

      void drawGuidlines(od::FrameBuffer &fb, int color) {
        auto height = mHeight - 1;
        auto halfHeight    = height / 2.0f;
        auto quarterHeight = height / 4.0f;
        auto eigthHeight   = height / 8.0f;
        auto center        = mWorldBottom + halfHeight;

        auto left = mWorldLeft;
        fb.hline(color, left, left + 1, center + quarterHeight + eigthHeight);
        fb.hline(color, left, left + 3, center + quarterHeight);
        fb.hline(color, left, left + 1, center + eigthHeight);
        fb.hline(color, left, left + 1, center - eigthHeight);
        fb.hline(color, left, left + 3, center - quarterHeight);
        fb.hline(color, left, left + 1, center - quarterHeight - eigthHeight);

        auto right = mWorldLeft + mWidth - 1;
        fb.hline(color, right, right - 1, center + quarterHeight + eigthHeight);
        fb.hline(color, right, right - 3, center + quarterHeight);
        fb.hline(color, right, right - 1, center + eigthHeight);
        fb.hline(color, right, right - 1, center - eigthHeight);
        fb.hline(color, right, right - 3, center - quarterHeight);
        fb.hline(color, right, right - 1, center - quarterHeight - eigthHeight);
      }

      void clearObjects() {
        if (mpThreshold) {
          mpThreshold->release();
          mpThreshold = 0;
        }
      }
  };

}
