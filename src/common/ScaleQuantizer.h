#pragma once

#include "util.h"
#include <hal/constants.h>
#include "ScaleBook.h"

namespace common {
  struct Pitch {
    inline Pitch() {}
    inline Pitch(int o, float c) : octave(o), cents(c) { }

    static inline Pitch from(int o, float c) {
      return Pitch { o, c };
    }

    static inline Pitch from(float value) {
      float voltage = FULLSCALE_IN_VOLTS * value;
      int octave = util::fdr(voltage);
      return from(octave, (voltage - (float)octave) * CENTS_PER_OCTAVE);
    }

    inline float value() {
      return (octave + cents / CENTS_PER_OCTAVE) / FULLSCALE_IN_VOLTS;
    }

    int octave = 0;
    float cents = 0;
  };

  struct QuantizedPitch {
    inline QuantizedPitch() {}
    inline QuantizedPitch(float cents, float lower, float upper) :
      mCents(cents),
      mLowerBound(lower),
      mUpperBound(upper) { }

    bool inBounds(float cents) const {
      return cents > mLowerBound && cents < mUpperBound;
    }

    Pitch atOctave(int o) const {
      return Pitch::from(o, mCents);
    }

    float wrappedCents() const {
      return util::fmod(mCents, CENTS_PER_OCTAVE);
    }

    float mCents = 0;
    float mLowerBound = 2 * CENTS_PER_OCTAVE;
    float mUpperBound = -CENTS_PER_OCTAVE;
  };

  template<int SPAN>
  class Quantizer {
    public:
      inline Quantizer() {}

      float process(float value) {
        mDetected = Pitch::from(value);
        if (mQuantized.inBounds(mDetected.cents))
          return mQuantized.atOctave(mDetected.octave).value();

        int upperIndex = 0;
        for (; upperIndex < SPAN * 2; upperIndex++) {
          if (mCentValues[upperIndex] > mDetected.cents) break;
        }

        auto lowerIndex = upperIndex - 1;

        auto upperDist = fabs(mCentValues[upperIndex] - mDetected.cents);
        auto lowerDist = fabs(mCentValues[lowerIndex] - mDetected.cents);

        auto closestIndex = upperDist < lowerDist ? upperIndex : lowerIndex;
        auto closestValue = mCentValues[closestIndex];

        auto lowerBound = closestValue + (mCentValues[closestIndex - 1] - closestValue) / 2.0f;
        auto upperBound = closestValue + (mCentValues[closestIndex + 1] - closestValue) / 2.0f;

        mQuantized = QuantizedPitch { closestValue, lowerBound, upperBound };
        return mQuantized.atOctave(mDetected.octave).value();
      }

      void configure(const Scale &scale) {
        int total  = scale.size();
        int note   = 0;
        int octave = 0;

        for (int i = 0; i < SPAN; i++) {
          auto centsUp   = scale.getCentValue(note);
          auto centsDown = scale.getCentValue(total - 1 - note);

          auto offset = CENTS_PER_OCTAVE * octave;
          mCentValues[SPAN + i]     = centsUp + offset;
          mCentValues[SPAN - 1 - i] = centsDown - (offset + CENTS_PER_OCTAVE);

          note += 1;
          if (note >= total) {
            note = 0;
            octave += 1;
          }
        }
      }

      const Pitch& detected() const { return mDetected; }
      const QuantizedPitch& quantized() const { return mQuantized; }

    private:
      float mCentValues[SPAN * 2];
      Pitch mDetected;
      QuantizedPitch mQuantized;
  };

  class ScaleQuantizer {
    public:
      inline ScaleQuantizer() {}

      void setCurrent(int i) {
        mIndex = i;
        mRefresh = true;
      }

      const Scale& current() const {
        return *mCurrent;
      }

      float detectedCentValue() const {
        return mQuantizer.detected().cents;
      }

      float detectedOctaveValue() const {
        return mQuantizer.detected().octave;
      }

      float quantizedCentValue() const {
        return mQuantizer.quantized().wrappedCents();
      }

      inline float process(float value) {
        prepare();
        return mQuantizer.process(value);
      }

      void prepare() {
        if (!mRefresh) return;
        mCurrent = &mScaleBook.scale(mIndex);
        mRefresh = false;
        mQuantizer.configure(current());
      }

    private:
      ScaleBook mScaleBook;
      int mIndex = 0;
      bool mRefresh = true;
      Scale const* mCurrent = 0;
      Quantizer<128> mQuantizer;
  };
}