#pragma once

#include "util.h"
#include <hal/constants.h>
#include "ScaleBook.h"

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace common {
  struct Pitch {
    inline Pitch() {}
    inline Pitch(int o, float c) : octave(o), cents(c) { }

    static inline Pitch from(int o, float c) {
      return Pitch { o, c };
    }

    static inline Pitch from(float value) {
      float voltage = util::toVoltage(value);
      float octave  = util::toOctave(voltage);
      return from(octave, util::toCents(voltage - octave));
    }

    inline float value() {
      return util::fromVoltage(octave + util::fromCents(cents));
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

    inline void resetBounds() {
      mLowerBound = 2 * CENTS_PER_OCTAVE;
      mUpperBound = -CENTS_PER_OCTAVE;
    }

    inline bool inBounds(float cents) const {
      return cents > mLowerBound && cents < mUpperBound;
    }

    inline Pitch atOctave(int o) const {
      return Pitch::from(o, mCents);
    }

    inline float wrappedCents() const {
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

      inline QuantizedPitch quantize(const Pitch &detected) const {
        // Binary search for the upper bound.
        int l = 0, h = SPAN - 1;
        while (l < h) {
          auto mid = (l + h) / 2;
          if (mCentValues[mid] > detected.cents) h = mid;
          else l = mid + 1;
        }

        return at(closest(detected.cents, h, h - 1));
      }

      // The quantized value at index i;
      inline QuantizedPitch at(int i) const {
        auto v = mCentValues[i];
        auto l = mCentValues[i - 1];
        auto h = mCentValues[i + 1];
        return QuantizedPitch { v, (v + h) / 2.0f, (v + l) / 2.0f };
      }

      // The closest index to v, a or b
      inline int closest(float v, int a, int b) const {
        auto distA = util::fabs(mCentValues[a] - v);
        auto distB = util::fabs(mCentValues[b] - v);
        return distA < distB ? a : b;
      }

      inline float quantizeValue(float value) const {
        if (mDisabled) return value;

        auto detected = Pitch::from(value);
        return quantize(detected).atOctave(detected.octave).value();
      }

      inline float process(float value) {
        mDetected = Pitch::from(value);
        if (mDisabled) return value;

        if (!mQuantized.inBounds(mDetected.cents))
          mQuantized = quantize(mDetected);

        return mQuantized.atOctave(mDetected.octave).value();
      }

      inline void configure(const Scale &scale) {
        mDisabled = scale.isEmpty();
        if (mDisabled) return;
        mQuantized.resetBounds();

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

      const Pitch& detected() const { return mDetected; }
      const QuantizedPitch& quantized() const { return mQuantized; }

    private:
      bool mDisabled = true;
      float mCentValues[SPAN];
      Pitch mDetected;
      QuantizedPitch mQuantized;
  };

  class ScaleQuantizer {
    public:
      inline ScaleQuantizer() {
        prepare();
      }

      inline void setCurrent(int i) {
        auto newIndex = util::mod(i, mScaleBook.size());
        mRefresh = mIndex != newIndex;
        mIndex = newIndex;
      }

      inline const Scale& current() const {
        return *mCurrent;
      }

      inline float detectedCentValue() const {
        return mQuantizer.detected().cents;
      }

      inline float detectedOctaveValue() const {
        return mQuantizer.detected().octave;
      }

      inline float quantizedCentValue() const {
        return mQuantizer.quantized().wrappedCents();
      }

      inline float quantizeValue(float value) const {
        return mQuantizer.quantizeValue(value);
      }

      inline float process(float value) {
        prepare();
        return mQuantizer.process(value);
      }

      inline void prepare() {
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