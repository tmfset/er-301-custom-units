#pragma once

#include <hal/simd.h>
#include <util/math.h>

namespace dsp {
  /**
    * Convert a gate to a trigger.
    * 
    * Each time the input goes high output a single trigger.
    */
  struct GateToTrigger {
    inline uint32_t process(const uint32_t signal) {
      auto trigger = mArmed & signal;
      mArmed = ~signal;
      return trigger;
    }

    uint32_t mArmed = ~0;
  };

  /**
    * A set-biased set/reset latch.
    * 
    * The latch goes high and stays high with the set input. It goes low
    * again only when reset goes high.
    */
  struct SRLatch {
    inline uint32_t read(const uint32_t set, const uint32_t reset) {
      mState = mState & ~reset;
      mState = mState | set;
      return mState;
    }

    uint32_t mState = 0;
  };

  namespace two {
    /**
      * A set-biased set/reset latch.
      * 
      * The latch goes high and stays high with the set input. It goes low
      * again only when reset goes high.
      */
    struct SRLatch {
      uint32x2_t process(uint32x2_t set, uint32x2_t reset) {
        mState = vand_u32(mState, vmvn_u32(reset));
        mState = vorr_u32(mState, set);
        return mState;
      }

      /**
        * The current state of the latch.
        */
      uint32x2_t mState = vdup_n_u32(0);
    };

    struct SyncedCountLatch {
      uint32x2_t process(uint32x2_t signal, uint32x2_t sync) {
        auto armed = mArmed.process(signal, vcge_u32(mCount, mMax));
        mCount = vbsl_u32(vand_u32(armed, sync), vadd_u32(mCount, vdup_n_u32(1)), vdup_n_u32(0));
        return vcgt_u32(mCount, vdup_n_u32(0));
      }

      SRLatch mArmed;
      uint32x2_t mMax = vdup_n_u32(1);
      uint32x2_t mCount = vdup_n_u32(0);
    };
  }

  namespace four {
    /**
      * Convert a gate to a trigger.
      * 
      * Each time the input goes high output a single trigger.
      */
    struct GateToTrigger {
      uint32x4_t process(uint32x4_t signal) {
        auto trigger = mArmed & signal;
        mArmed = ~signal;
        return trigger;
      }

      /**
        * Is the mechanism armed to emit a trigger on the next signal high?
        */
      uint32x4_t mArmed = ~vdupq_n_u32(0);
    };

    /**
      * A set-biased set/reset latch.
      * 
      * The latch goes high and stays high with the set input. It goes low
      * again only when reset goes high.
      */
    struct SRLatch {
      uint32x4_t process(uint32x4_t set, uint32x4_t reset) {
        mState = mState & ~reset;
        mState = mState | set;
        return mState;
      }

      /**
        * The current state of the latch.
        */
      uint32x4_t mState = vdupq_n_u32(0);
    };

    /**
      * Basic track and hold, track a signal and hold it on the input gate.
      */
    struct TrackAndHold {
      inline TrackAndHold(float initial) {
        mValue = vdupq_n_f32(initial);
      }

      inline void set(const float32x4_t signal) {
        mValue = signal;
      }

      inline void track(const uint32x4_t gate, const float32x4_t signal) {
        mValue = vbslq_f32(gate, signal, mValue);
      }

      inline void track(const uint32x4_t gate, const TrackAndHold &other) {
        track(gate, other.value());
      }

      inline float32x4_t value() const {
        return mValue;
      }

      float32x4_t mValue = vdupq_n_f32(0);
    };

    struct SyncedCountLatch {
      uint32x4_t process(uint32x4_t signal, uint32x4_t sync) {
        auto armed = mArmed.process(signal, vmvnq_u32(signal) & sync);
        //mCount += (armed & sync) & vdupq_n_u32(1);
        mCount = armed & (mCount + (sync & vdupq_n_u32(1)));
        return state();
      }

      uint32x4_t state() {
        return vcgtq_u32(mCount, vdupq_n_u32(0));
      }

      SRLatch mArmed;
      //uint32x4_t mMax = vdupq_n_u32(10);
      uint32x4_t mCount = vdupq_n_u32(0);
    };

    struct SyncTrigger {
      inline uint32x4_t read(uint32x4_t high, uint32x4_t sync) {
        auto trigger = mTrigger.process(high);
        mLatch = mLatch | trigger; // set
        auto out = mLatch & sync;
        mLatch = mLatch & ~sync; // reset
        return out;
      }

      uint32x4_t mLatch = vdupq_n_u32(0);
      GateToTrigger mTrigger;
    };
  }
}