#pragma once

#include <od/graphics/sampling/TapeHeadDisplay.h>
#include <math.h>
#include <Sloop2.h>
#include <util/math.h>
#include <graphics/primitives/Box.h>
#include <graphics/primitives/Point.h>
#include <util/v2d.h>

namespace sloop {
  class SloopHeadMainDisplay : public od::TapeHeadDisplay {
    public:
      SloopHeadMainDisplay(Sloop2 *head, int left, int bottom, int width, int height) :
        od::TapeHeadDisplay(head, left, bottom, width, height) {
          mSampleView.setViewTimeInSeconds(2);
        }

      virtual ~SloopHeadMainDisplay() {}

#ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb) {
        uint32_t pos = 0;
        Sloop2 *pRecordHead = sloopHead();
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

      struct JumpAnimation {
        int mState = 0;
        int mDelay = 40;
        float mIntensity = 7.0f;
        v2d mStep = v2d::ofX(2);
        int mIsIndicatingLeft = false;

        int mArrowCount = 2;
        int mArrowSize = 5;

        void reset() {
          mState = 0;
        }

        void play(od::FrameBuffer &fb, Sloop2 &sloop, v2d position) {
          if (!sloop.isJumpPending()) {
            reset();
            return;
          }

          int currentPosition = sloop.getPosition();
          int jumpPosition    = sloop.jumpPosition();
          int sampleLength    = sloop.getSampleLength();

          int leftDistance = util::mod(currentPosition - jumpPosition, sampleLength);
          int rightDistance = util::mod(jumpPosition - currentPosition, sampleLength);
          bool isIndicatingLeft = leftDistance < rightDistance;

          // If we switch directions reset the animation.
          if (mIsIndicatingLeft && !isIndicatingLeft) reset();
          if (!mIsIndicatingLeft && isIndicatingLeft) reset();
          mIsIndicatingLeft = isIndicatingLeft;

          //auto pos = isIndicatingLeft ? position.leftCenter() : position.rightCenter();
          auto offset = isIndicatingLeft ? -mStep : mStep;
          auto symbol = isIndicatingLeft ? "<" : ">";
          for (int i = 0; i < mArrowCount; i++) {
            float stage = (mDelay / 2.0f) * (i + 1) / (float)mArrowCount;
            float fade = powf((mDelay - util::moddst(mState, stage, mDelay)) / (float)mDelay, mIntensity);
            float color = WHITE * fade;

            auto point = graphics::Point::of(position);
            if (isIndicatingLeft) point.arrowLeft(fb, color, mArrowSize);
            else                  point.arrowRight(fb, color, mArrowSize);

            position = position + offset;
          }

          mState += 1;
        }

        void drawArrow(od::FrameBuffer &fb, v2d center, int halfSize, bool left) {
          int size = halfSize * 2 + 1;
          
        }
      };

      JumpAnimation mJumpAnimation;

      Sloop2 *sloopHead() {
        return (Sloop2 *)mpHead;
      }

      static char const * playHeadLetter(Sloop2::State state) {
        switch (state) {
          case Sloop2::Hold:        return "H";
          case Sloop2::Write:       return "W";
          case Sloop2::PlayReverse: return "R";
          default:                  return "F";
        }
      }

      static int playHeadSize(Sloop2::State state) {
        switch (state) {
          case Sloop2::Hold:        return 6;
          case Sloop2::Write:       return 8;
          case Sloop2::PlayReverse: return 6;
          default:                  return 5;
        }
      }

      void drawCursor(od::FrameBuffer &fb) {
        auto sloop = sloopHead();
        if (!sloop) return;

        int position = sloop->getPosition();
        float level = sloop->writeLevel();
        auto state = sloop->state();
        auto size = playHeadSize(state);
        mSampleView.drawPosition(fb, position, playHeadLetter(state), size);

        float x0 = getPositionX(position);
        float y0 = mWorldBottom + mHeight / 2;

        mJumpAnimation.play(fb, *sloop, v2d::of(x0, y0));
        //drawResetIndicator(fb);

        if (!isPositionInBounds(position)) return;
        if (level == 0) return;

        int   width = 2;
        float height = (mHeight - 2) / 4.0f;

        float offset = level * (float)height;
        fb.vline(GRAY7, x0, y0 - offset, y0 + offset);
        fb.hline(WHITE, x0 - width / 2, x0 + width / 2, y0 + offset);
        fb.hline(WHITE, x0 - width / 2, x0 + width / 2, y0 - offset);
      }

      void drawClockMarks(od::FrameBuffer &fb, int height, int resolution, int color) {
        auto sloop = sloopHead();
        if (!sloop) return;

        //drawClockMark(fb, 1, height, color);

        auto length = sloop->getSampleLength();
        auto period = sloop->getInternalClockPeriod();

        auto total = length / period;

        for (int i = 0; i <= total; i++) {
          float h = height;
          int   c = color;

          // Scale down the intermediate marks.
          for (int r = 1; r <= resolution; r++) {
            if (i % (int)pow(2, r)) { h /= 2.0f; c -=2; }
          }

          drawClockMark(fb, i * period, h, c);
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