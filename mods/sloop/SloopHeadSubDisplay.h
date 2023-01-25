#pragma once

#include <od/graphics/Graphic.h>
#include <od/graphics/constants.h>
#include <od/audio/Sample.h>
#include <Sloop2.h>

namespace sloop {
  class SloopHeadSubDisplay : public od::Graphic {
    public:
      SloopHeadSubDisplay(Sloop2 *sloop) :
        od::Graphic(0, 0, 128, 64) {
        mpSloop = sloop;
        if (mpSloop) mpSloop->attach();
      }
      
      virtual ~SloopHeadSubDisplay() {
        if (mpSloop) mpSloop->release();
        mpSloop = 0;
      }

    private:
      Sloop2 *mpSloop = 0;
      std::string mHeadText { "00:00.000" };
      std::string mLoopText { "00:00.000" };

      static void timeString(float totalSecs, std::string &result) {
        int hours = totalSecs / 3600;
        int mins  = totalSecs / 60;
        int secs  = totalSecs;

        totalSecs -= hours * 3600;
        totalSecs -= mins * 60;
        totalSecs -= secs;

        int ms = 1000 * totalSecs;
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%02d:%02d.%03d", mins, secs, ms);
        result = tmp;
      }

      void updateText() {
        if (!mpSloop) return;

        od::Sample *pSample = mpSloop->getSample();
        if (!pSample) return;

        int head = mpSloop->getPosition();
        int loop = mpSloop->lastPosition();
        float sr = pSample->mSampleRate;

        timeString(head / sr, mHeadText);
        timeString(loop / sr, mLoopText);
      }

      void draw(od::FrameBuffer &fb) {
        updateText();

        fb.text(WHITE, 5, 40, "head:", 10);
        fb.text(WHITE, 5, 30, "length:", 10);

        fb.text(WHITE, 83, 40, mHeadText.c_str(), 10);
        fb.text(WHITE, 83, 30, mLoopText.c_str(), 10);
      }
  };
}