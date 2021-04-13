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

      //inline void readNeonSync(in32_t *out, uint32x4_t high)

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
        if (mState) mEnable = true;
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

      inline bool readOpt(bool high, bool sync) {
        if (mState) mEnable = true;
        else        mEnable = (mEnable || !high) && !mTrigger;

        if (mEnable) mTrigger = high;
        if (sync)    mState   = mTrigger;

        return mState;
      }

      bool state() { return mState; }

    private:
      bool mEnable  = true;
      bool mTrigger = false;
      bool mState   = false;
  };
}