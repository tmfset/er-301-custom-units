#pragma once

#include <od/graphics/sampling/TapeHeadDisplay.h>
#include <math.h>
#include <Sloop.h>

namespace sloop {
  class SloopHeadDisplay : public od::TapeHeadDisplay {
    public:
      SloopHeadDisplay(Sloop *head, int left, int bottom, int width, int height) :
        od::TapeHeadDisplay(head, left, bottom, width, height) {
          mSampleView.setViewTimeInSeconds(2);
        }

      virtual ~SloopHeadDisplay() {}

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

      void drawCursor(od::FrameBuffer &fb) {
        auto sloop = sloopHead();
        if (!sloop) return;

        int position = sloop->getPosition();
        float level = sloop->recordLevel();
        auto state = sloop->state();
        mSampleView.drawPosition(fb, position, playHeadLetter(state), playHeadSize(state));

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

        auto length = sloop->numberOfClockMarks();
        for (int i = 0; i < length; i++) {
          float h = height;
          int   c = color;

          // Scale down the intermediate marks.
          for (int r = 1; r <= resolution; r++) {
            if (i % (int)pow(2, r)) { h /= 2.0f; c -=2; }
          }

          drawClockMark(fb, sloop->getClockMark(i), h, c);
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