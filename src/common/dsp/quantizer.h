#pragma once

#include <vector>
#include <string>

#include <hal/constants.h>
#include <od/config.h>

#include "../util.h"
#include "../v2d.h"

namespace common {
  struct Scale {
    inline std::string name() const { return mName; }
    inline bool isEmpty() const { return size() == 0; }
    inline int size() const { return mSize; }
    inline float getCentValue(int i) const { return mCentValues[i]; }

    std::string mName;
    int mSize;
    float mCentValues[24];
  };

  class ScaleBook {
    public:
      inline ScaleBook() { }

      // void addPitch(float cents) {
      //   mWrite.push_back(cents);
      // }

      // void commitScale(std::string name) {
      //   //Scale scale { name, mWrite.size(), mWrite. };
      //   //mScales.push_back(scale);
      //   mWrite.clear();
      // }

      inline int size() const {
        return mScales.size();
      }

      const Scale& scale(int i) const {
        return mScales.at(i);
      }

    private:
      std::vector<float> mWrite;
      std::vector<Scale> mScales = {
        { "OFF",         0, { } },

        { "12-T",     12, { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 } },
        { "24-T",     24, { 0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150 } },

        { "PENT+", 5, { 0, 200, 400, 700, 900 } },
        { "PENT-", 5, { 0, 300, 500, 700, 1000} },

        { "NAT-",  7, { 0, 200, 300, 500, 700, 800, 1000, 1200 } },
        { "HARM-", 7, { 0, 200, 300, 500, 700, 800, 1100, 1200 } },

        { "ION",      7, { 0, 200, 400, 500, 700, 900, 1100 } }, // IONIAN
        { "DOR",      7, { 0, 200, 300, 500, 700, 900, 1000 } }, // DORIAN
        { "PHRY",    7, { 0, 100, 300, 500, 700, 800, 1000 } }, // PHRYGIAN
        { "LYD",      7, { 0, 200, 400, 600, 700, 900, 1100 } }, // LYDIAN
        { "MIX",  7, { 0, 200, 400, 500, 700, 800, 1000 } }, // MIXOLYDIAN
        { "AEOL",     7, { 0, 200, 300, 500, 700, 800, 1000 } }, // AEOLIAN
        { "LOC",     7, { 0, 100, 300, 500, 600, 800, 1000 } }, // LOCRIAN

        { "WHL",  6, { 0, 200, 400, 600, 800, 1000 } }
      };
  };

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