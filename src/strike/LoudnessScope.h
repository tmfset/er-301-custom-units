#pragma once

#include <od/objects/measurement/FifoProbe.h>
#include <od/graphics/Graphic.h>
#include <od/graphics/text/Label.h>
#include <od/extras/FastEWMA.h>
#include <od/AudioThread.h>

namespace strike {
  class LoudnessScope : public od::Graphic {
    public:
      LoudnessScope(
        int left,
        int bottom,
        int width,
        int height
      ) : Graphic(left, bottom, width, height) {
        mMaximums.resize(mWidth + 1);
        mMinimums.resize(mWidth + 1);
        for (int i = 0; i < mWidth; i++) {
          mMaximums[i] = 0;
          mMinimums[i] = 0;
        }
        mShowStatus = mWidth > 50;
      }

      virtual ~LoudnessScope() {
        disconnectProbe();
        clearOutlets();
      }

#ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb);
#endif

      void watchOutlet(od::Outlet *outlet) {
        disconnectProbe();
        clearOutlets();
        mpWatchedOutlet = outlet;
        if (mpWatchedOutlet) {
          mpWatchedOutlet->attach();
          if (mVisibility == od::visibleState) {
            connectProbe();
          }
        }
      }

      void watchInlet(od::Inlet *inlet) {
        disconnectProbe();
        clearOutlets();
        mpWatchedInlet = inlet;
        if (mpWatchedInlet) {
          mpWatchedInlet->attach();
          if (mVisibility == od::visibleState) {
            connectProbe();
          }
        }
      }

      void setOffset(float offset) { mOffset = offset; }
      void setLeftBorder(int width) { mLeftBorder = width; }
      void setRightBorder(int width) { mRightBorder = width; }
      void showStatus(bool value) { mShowStatus = value; }

    protected:
      od::FifoProbe *mpProbe = 0;
      od::Outlet *mpWatchedOutlet = 0;
      od::Inlet *mpWatchedInlet = 0;

      float mTriggerThreshold = 0.0f;
      float mOffset = 0.0f;
      std::vector<int> mMaximums;
      std::vector<int> mMinimums;

      void calculate();
      int mCalculateCount = 0;
      static const int RefreshTime = 2;
      static const int WarmUpTime = 10;

      int mLeftBorder = 0;
      int mRightBorder = 0;
      bool mShowStatus;

      void connectProbe() {
        if (mpProbe) return;

        if (mpWatchedOutlet) {
          mpProbe = od::AudioThread::getFifoProbe();
          if (mpProbe) {
            od::AudioThread::connect(mpWatchedOutlet, &mpProbe->mInput);
          }
        } else if (mpWatchedInlet && mpWatchedInlet->mInwardConnection) {
          mpProbe = od::AudioThread::getFifoProbe();
          if (mpProbe) {
            od::AudioThread::connect(mpWatchedInlet->mInwardConnection, &mpProbe->mInput);
          }
        }

        mCalculateCount = -WarmUpTime;
      }

      void disconnectProbe() {
        if (mpProbe) {
          od::AudioThread::disconnect(&mpProbe->mInput);
          od::AudioThread::releaseFifoProbe(mpProbe);
          mpProbe = 0;
        }
      }

      void clearOutlets() {
        if (mpWatchedOutlet) {
          mpWatchedOutlet->release();
          mpWatchedOutlet = 0;
        }

        if (mpWatchedInlet) {
          mpWatchedInlet->release();
          mpWatchedInlet = 0;
        }
      }

      virtual void notifyHidden() {
        //logDebug(10, "LoudnessScope(%s): hidden", getWatchName().c_str());
        disconnectProbe();
        Graphic::notifyHidden();
      }

      virtual void notifyVisible() {
        //logDebug(10, "LoudnessScope(%s): visible", getWatchName().c_str());
        connectProbe();
        Graphic::notifyVisible();
      }

      std::string getWatchName() {
        if (mpWatchedOutlet) {
          return mpWatchedOutlet->mName;
        } else if (mpWatchedInlet) {
          return mpWatchedInlet->mName;
        }
        return "None";
      }
  };

}
