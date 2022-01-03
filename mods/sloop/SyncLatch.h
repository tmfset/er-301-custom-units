#pragma once

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace sloop {
  class SyncLatch {
    public:
      SyncLatch() {}

      SyncLatch(const SyncLatch &other) {
        this->mEnable  = other.mEnable;
        this->mTrigger = other.mTrigger;
        this->mState   = other.mState;
      }

      inline bool readSyncCount(bool high, bool sync) {
        mCount += mState && sync;

        if (mState) mEnable = true;
        else        mEnable = (mEnable || !high) && !mTrigger;

        if (mEnable) mTrigger = high;
        if (sync)    mState   = mTrigger;

        return mState;
      }

      inline bool readSyncCountMax(bool high, bool sync, int max) {
        mCount = mState ? mCount + sync : 0;

        if (mState) mEnable = mCount >= max;
        else        mEnable = (mEnable || !high) && !mTrigger;

        if (mEnable) mTrigger = high;
        if (sync)    mState   = mTrigger;

        return mState;
      }

      inline bool readSync(bool high, bool sync) {
        mEnable = (mEnable || !high) && !mTrigger;

        if (mEnable) mTrigger = high;
        if (sync)    mState   = mTrigger;

        return mState;
      }

      inline bool read(bool high) {
        if (mState) mEnable = true;
        else        mEnable = mEnable || !high;

        if (mEnable) mState = mTrigger = high;

        return mState;
      }

      inline bool readGateSyncCount(bool high, bool sync) {
        mCount += mState && sync;
        mTrigger = (sync && high) || (!sync && mState);
        mState   = mTrigger;
        mEnable  = true;
        return mState;
      }

      inline bool readTrigger(bool high) {
        mTrigger = mEnable && high;
        mState   = mTrigger;
        mEnable  = !mTrigger && !high;
        return mState;
      }

      inline bool readTriggerSync(bool high, bool sync) {
        mTrigger = (mEnable && high) || (mTrigger && !mState);
        mState   = sync && mTrigger;
        mEnable  = (!mTrigger && !high) || mState;
        return mState;
      }

      inline bool state() const     { return mState; }
      inline bool triggered() const { return mTrigger; }

      inline int  count() const       { return mCount; }
      inline void setCount(int count) { mCount = count; }

      inline bool first() const       { return mCount == 0; }
      inline bool firstOrLast() const { return mState ? first() : !first(); }

    private:
      int  mCount   = 0;
      bool mEnable  = true;
      bool mTrigger = false;
      bool mState   = false;
  };
}