#pragma once

#include <od/graphics/sampling/TapeHeadDisplay.h>
#include <math.h>
#include <Sloop.h>
#include <util.h>

namespace sloop {
  class SloopHeadMainDisplay : public od::TapeHeadDisplay {
    public:
      SloopHeadMainDisplay(Sloop *head, int left, int bottom, int width, int height) :
        od::TapeHeadDisplay(head, left, bottom, width, height) {
          mSampleView.setViewTimeInSeconds(2);
        }

      virtual ~SloopHeadMainDisplay() {}

#ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb) {
        uint32_t pos = 0;
        Sloop *pRecordHead = sloopHead();
        if (pRecordHead) {
          od::Sample *pSample = pRecordHead->getSample();
          pos = pRecordHead->getPosition();

          if (mSampleView.setSample(pSample)) {
              // new sample, so do nothing
          } else if (mLastRecordPosition != pos) {
            if (mLastRecordPosition < pos) {
              mSampleView.invalidateInterval(mLastRecordPosition, pos);
            } else {
              // wrapped
              mSampleView.invalidateInterval(mLastRecordPosition, pSample->mSampleCount);
              mSampleView.invalidateInterval(0, pos);
            }
          }

          mLastRecordPosition = pos;
        }

        drawClockMarks(fb, mHeight / 16, 2, GRAY13);

        mSampleView.setCenterPosition(pos);
        mSampleView.draw(fb);
        mSampleView.drawPositionOverview(fb, pos);

        drawCursor(fb);

        switch (mZoomGadgetState) {
          case showTimeGadget:
            mSampleView.drawTimeZoomGadget(fb);
            mCursorState.copyAttributes(mSampleView.mCursorState);
            break;

          case showGainGadget:
            mSampleView.drawGainZoomGadget(fb);
            mCursorState.copyAttributes(mSampleView.mCursorState);
            break;

          default:
            break;
        }
      }

      uint32_t mLastRecordPosition = 0;
#endif

    private:
      int mResetAnimationState = 0;
      bool mWasResetAnimatedLeft = false;

      Sloop *sloopHead() {
        return (Sloop *)mpHead;
      }

      char const * playHeadLetter(Sloop::State state) {
        switch (state) {
          case Sloop::Waiting:     return "W";
          case Sloop::Recording:   return "R";
          case Sloop::Overdubbing: return "O";
          default:                 return "P";
        }
      }

      int playHeadSize(Sloop::State state) {
        switch (state) {
          case Sloop::Waiting:     return 8;
          case Sloop::Recording:   return 6;
          case Sloop::Overdubbing: return 6;
          default:                 return 5;
        }
      }

      void drawResetIndicator(od::FrameBuffer &fb) {
        auto sloop = sloopHead();
        if (!sloop) return;

        if (!sloop->isResetPending()) {
          mResetAnimationState = 0;
          return;
        }

        // int currentStep = sloop->getCurrentStep();
        // int resetStep   = sloop->getResetStep();
        // int dist = abs(currentStep - resetStep);
        // int n = 1 + (4.0f * fmin(dist / (float)((sloop->numberOfClockMarks() / 2.0f) + 1), 1.0f));
        int n = 3;

        int current  = sloop->getPosition();
        int reset    = sloop->resetPosition();
        int isBefore = reset < current;

        if (mWasResetAnimatedLeft && !isBefore) mResetAnimationState = 0;
        if (!mWasResetAnimatedLeft && isBefore) mResetAnimationState = 0;
        mWasResetAnimatedLeft = isBefore;

        auto state    = sloop->state();
        auto headSize = playHeadSize(state);
        float halfHead = headSize / 2.0f;

        float pps = mSampleView.mPixelsPerSample;
        int x0  = mWorldLeft + (int)(pps * (current - mSampleView.mStart));
        int y0  = mWorldBottom + mHeight - 10;
        int offset = isBefore ? -(halfHead + 5) : halfHead + 2;

        int s = mResetAnimationState;
        int delay = 30;
        int intensity = 7.0f;
        int step = 2;
        for (int i = 0; i < n; i++) {
          float stage = (delay / 2.0f) * (i / (float)(n - 1));
          float fade  = powf((delay - util::moddst(s, stage, delay)) / (float)delay, intensity);
          float color = WHITE * fade;
          fb.text(color, x0 + offset, y0, isBefore ? "<" : ">", 10);
          offset += isBefore ? -step : step;
        }

        mResetAnimationState += 1;
      }

      void drawCursor(od::FrameBuffer &fb) {
        auto sloop = sloopHead();
        if (!sloop) return;

        int position = sloop->getPosition();
        float level = sloop->writeLevel();
        auto state = sloop->state();
        auto size = playHeadSize(state);
        mSampleView.drawPosition(fb, position, playHeadLetter(state), size);

        drawResetIndicator(fb);

        if (!isPositionInBounds(position)) return;
        if (level == 0) return;

        int   width = 2;
        float height = (mHeight - 2) / 4.0f;
        float x0 = getPositionX(position);
        float y0 = mWorldBottom + mHeight / 2;
        float offset = level * (float)height;
        fb.vline(GRAY7, x0, y0 - offset, y0 + offset);
        fb.hline(WHITE, x0 - width / 2, x0 + width / 2, y0 + offset);
        fb.hline(WHITE, x0 - width / 2, x0 + width / 2, y0 - offset);
      }

      void drawClockMarks(od::FrameBuffer &fb, int height, int resolution, int color) {
        auto sloop = sloopHead();
        if (!sloop) return;

        drawClockMark(fb, 1, height, color);

        auto length = sloop->visibleMarks();
        for (int i = 0; i < length; i++) {
          float h = height;
          int   c = color;

          // Scale down the intermediate marks.
          for (int r = 1; r <= resolution; r++) {
            if (i % (int)pow(2, r)) { h /= 2.0f; c -=2; }
          }

          drawClockMark(fb, sloop->markAt(i), h, c);
        }
      }

      void drawClockMark(od::FrameBuffer &fb, int position, int height, int color) {
        if (!isPositionInBounds(position)) return;

        int x0     = getPositionX(position);
        int bottom = mWorldBottom + 1;
        int top    = mWorldBottom + mHeight - 1;
        fb.vline(GRAY3, x0, bottom, top, 1);
        fb.vline(color, x0, bottom, bottom + height);
      }

      int getPositionX(int position) {
        return mWorldLeft + (int)(mSampleView.mPixelsPerSample * (position - mSampleView.mStart));
      }

      bool isPositionInBounds(int position) {
        return position < mSampleView.mEnd && position > mSampleView.mStart;
      }
  };
}