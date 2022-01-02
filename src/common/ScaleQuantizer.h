#pragma once

#include "util.h"
#include <hal/constants.h>
#include "ScaleBook.h"
#include "v2d.h"

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace common {
  template<int SPAN>
  class Quantizer {
    public:
      inline Quantizer() {}

      inline int quantizeToIndex(float cents) const {
        // Binary search for the upper bound.
        int l = 0, h = SPAN - 1;
        while (l < h) {
          auto mid = (l + h) / 2;
          if (mCentValues[mid] > cents) h = mid;
          else l = mid + 1;
        }

        return closest(cents, h, h - 1);
      }

      inline float quantize(float cents) const {
        return mCentValues[quantizeToIndex(cents)];
      }

      inline float valueAt(int i) const {
        return mCentValues[i];
      }

      inline v2d boundsAt(int i) const {
        auto base  = v2d::of(mCentValues[i]);
        auto edges = v2d::of(mCentValues[i - 1], mCentValues[i + 1]);
        return (edges + base) * 0.5f;
      }

      inline void configure(const Scale &scale) {
        int total  = scale.size();
        int note   = 0;
        int octave = 0;

        auto hspan = SPAN >> 1;
        for (int i = 0; i < hspan; i++) {
          auto centsUp   = scale.getCentValue(note);
          auto centsDown = scale.getCentValue(total - 1 - note);

          auto offset = CENTS_PER_OCTAVE * octave;
          mCentValues[hspan + i]     = centsUp + offset;
          mCentValues[hspan - 1 - i] = centsDown - (offset + CENTS_PER_OCTAVE);

          note += 1;
          if (note >= total) {
            note = 0;
            octave += 1;
          }
        }
      }

    private:
      // The closest index to v. Is it a or b?
      inline int closest(float v, int a, int b) const {
        auto distA = util::fabs(mCentValues[a] - v);
        auto distB = util::fabs(mCentValues[b] - v);
        return distA < distB ? a : b;
      }

      float mCentValues[SPAN];
  };

  class ScaleQuantizer {
    public:
      inline ScaleQuantizer() {
        prepare();
      }

      inline void setCurrent(int i) {
        auto newIndex = util::mod(i, mScaleBook.size());
        if (mIndex != newIndex) prepare();
        mIndex = newIndex;
      }

      inline const Scale& current() const {
        return *mCurrent;
      }

      inline int getCurrentIndex() const {
        return mIndex;
      }

      inline int getScaleBookSize() const {
        return mScaleBook.size();
      }

      inline const Scale& getScale(int i) const {
        return mScaleBook.scale(i);
      }

      inline float detectedCentValue()   const { return vgetq_lane_f32(mDetected.cents(), 3); }
      inline int   detectedOctaveValue() const { return vgetq_lane_f32(mDetected.octave(), 3); }
      inline float quantizedCentValue()  const { return util::fmod(mQuantizedCents, CENTS_PER_OCTAVE); }

      inline float quantize(float value) const {
        return mQuantizer.quantize(value);
      }

      inline float32x4_t process(float32x4_t value) {
        mDetected = util::four::Pitch::from(value);

        if (mDisabled)
          return value;

        if (allInBounds(mDetected.cents()))
          return mDetected.atCents(mQuantizedCents).value();

        float tmp[4];
        vst1q_f32(tmp, mDetected.cents());

        tmp[0] = mQuantizer.quantize(tmp[0]);
        tmp[1] = mQuantizer.quantize(tmp[1]);
        tmp[2] = mQuantizer.quantize(tmp[2]);

        auto i = mQuantizer.quantizeToIndex(tmp[3]);
        tmp[3] = mQuantizer.valueAt(i);
        mQuantizedCents = tmp[3];
        mQuantizedBounds = mQuantizer.boundsAt(i);

        return mDetected.atCents(vld1q_f32(tmp)).value();
      }

    private:
      inline void prepare() {
        mCurrent = &mScaleBook.scale(mIndex);

        mDisabled = mCurrent->isEmpty();
        if (mDisabled) return;

        mQuantizer.configure(current());
        mQuantizedBounds = v2d::of(0, 0);
      }

      inline bool allInBounds(float32x4_t cents) {
        return util::four::allInRange(cents, mQuantizedBounds.x(), mQuantizedBounds.y());
      }

      ScaleBook mScaleBook;
      int mIndex = 0;
      Scale const* mCurrent = 0;
      bool mDisabled = true;
      Quantizer<128> mQuantizer;

      util::four::Pitch mDetected;
      float mQuantizedCents;
      common::v2d mQuantizedBounds;
  };
}