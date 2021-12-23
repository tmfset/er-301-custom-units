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

    static inline QuantizedPitch from(float cents, float previous, float next) {
      return QuantizedPitch {
        cents,
        cents + (previous - cents) / 2.0f,
        cents + (next - cents) / 2.0f
      };
    }

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
        int upper = 0;
        for (; upper < SPAN * 2; upper++) {
          if (detected.cents < mCentValues[upper]) break;
        }

        auto lower = upper - 1;
        auto upperDist = util::fabs(mCentValues[upper] - detected.cents);
        auto lowerDist = util::fabs(mCentValues[lower] - detected.cents);

        auto index = upperDist < lowerDist ? upper : lower;
        auto value = mCentValues[index];

        return QuantizedPitch::from(value, mCentValues[index - 1], mCentValues[index + 1]);
      }

      inline float quantizeValue(float value) const {
        auto detected = Pitch::from(value);
        return quantize(detected).atOctave(detected.octave).value();
      }

      inline float process(float value) {
        mDetected = Pitch::from(value);
        if (mQuantized.inBounds(mDetected.cents))
          return mQuantized.atOctave(mDetected.octave).value();

        mQuantized = quantize(mDetected);
        return mQuantized.atOctave(mDetected.octave).value();
      }

      inline void configure(const Scale &scale) {
        if (scale.isEmpty()) return;
        mQuantized.resetBounds();

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

      inline void setCurrent(int i) {
        mIndex = util::mod(i, mScaleBook.size());
        mRefresh = true;
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