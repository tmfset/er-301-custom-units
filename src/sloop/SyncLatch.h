#pragma once

namespace sloop {
  class SyncLatch {
    public:
      SyncLatch() {}

      SyncLatch(const SyncLatch &other) {
        this->mEnable  = other.mEnable;
        this->mTrigger = other.mTrigger;
        this->mState   = other.mState;
      }

      inline void readVector(int32_t *out, uint32_t const * high) {
        for (int i = 0; i < 4; i++) {
          out[i] = read(high[i]);
        }
      }

      inline void readVectorSync(int32_t *out, uint32_t const * high, int32_t const * sync) {
        for (int i = 0; i < 4; i++) {
          out[i] = readSync(high[i], sync[i]);
        }
      }

      inline bool readSync(bool high, bool sync) {
        mCount = mState ? mCount + sync : 0;

        if (mState) mEnable = true;
        else        mEnable = (mEnable || !high) && !mTrigger;

        if (mEnable) mTrigger = high;
        if (sync)    mState   = mTrigger;

        return mState;
      }

      inline bool readSyncMax(bool high, bool sync, int max) {
        mCount = mState ? mCount + sync : 0;

        if (mState) mEnable = mCount >= max;
        else        mEnable = (mEnable || !high) && !mTrigger;

        if (mEnable) mTrigger = high;
        if (sync)    mState   = mTrigger;

        return mState;
      }

      inline bool read(bool high) {
        if (mTrigger) mEnable = true;
        else          mEnable = mEnable || !high;

        if (mEnable) mTrigger = mState = high;

        return mTrigger;
      }

      inline bool state() { return mState; }

      inline int count() { return mCount; }
      inline void setCount(int count) { mCount = count; }

      inline bool first() {
        return mCount == 0;
      }

      inline bool firstOrLast() {
        return mState ? mCount == 0 : mCount != 0;
      }

    private:
      int  mCount   = 0;
      bool mEnable  = true;
      bool mTrigger = false;
      bool mState   = false;
  };
}