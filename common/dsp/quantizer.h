#pragma once

#include <vector>
#include <string>

#include <hal/constants.h>
#include <od/config.h>

#include <dsp/pitch.h>
#include <dsp/timer.h>
#include <util/math.h>
#include <util/v2d.h>

namespace dsp {
  #define MAX_SCALE_SIZE 24

  struct Scale {
    inline std::string name()             const { return mShortName; }
    inline bool        isEmpty()          const { return size() == 0; }
    inline int         size()             const { return mSize; }
    inline float       centValueAt(int i) const { return mCentValues[i]; }

    std::string mShortName;
    std::string mLongName;
    int mSize;
    float mCentValues[MAX_SCALE_SIZE];

    static inline Scale off() { return { "OFF", "OFF", 0, { } }; }

    static inline Scale twelveTet()     { return { "12T", "12-TET", 12, { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 } }; }
    static inline Scale twentyFourTet() { return { "24T", "24-TET", 24, { 0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1050, 1100, 1150 } }; }

    static inline Scale pentatonicMajor() { return { "PT+", "PENTATONIC+", 5, { 0, 200, 400, 700, 900 } }; }
    static inline Scale pentatonicMinor() { return { "PT-", "PENTATONIC-", 5, { 0, 300, 500, 700, 1000} }; }

    static inline Scale naturalMinor()  { return { "NAT", "NATURAL-",  7, { 0, 200, 300, 500, 700, 800, 1000, 1200 } }; }
    static inline Scale harmonicMinor() { return { "HRM", "HARMONIC-", 7, { 0, 200, 300, 500, 700, 800, 1100, 1200 } }; }

    static inline Scale ionian()     { return { "ION", "IONIAN",     7, { 0, 200, 400, 500, 700, 900, 1100 } }; }
    static inline Scale dorian()     { return { "DOR", "DORIAN",     7, { 0, 200, 300, 500, 700, 900, 1000 } }; }
    static inline Scale phrygian()   { return { "PHR", "PHRYGIAN",   7, { 0, 100, 300, 500, 700, 800, 1000 } }; }
    static inline Scale lydian()     { return { "LYD", "LYDIAN",     7, { 0, 200, 400, 600, 700, 900, 1100 } }; }
    static inline Scale mixolydian() { return { "MIX", "MIXOLYDIAN", 7, { 0, 200, 400, 500, 700, 800, 1000 } }; }
    static inline Scale aeolian()    { return { "AEL", "AEOLIAN",    7, { 0, 200, 300, 500, 700, 800, 1000 } }; }
    static inline Scale locrian()    { return { "LOC", "LOCRIAN",    7, { 0, 100, 300, 500, 600, 800, 1000 } }; }

  };

  class ScaleBuilder {
    public:
      inline void write(float cents) {
        mWrite.push_back(cents);
      }

      inline Scale build(std::string name, std::string abbreviation) {
        auto size = util::min(mWrite.size(), MAX_SCALE_SIZE);
        auto scale = Scale { name, abbreviation, size, { } };
        for (int i = 0; i < size; i++) scale.mCentValues[i] = mWrite.at(i);
        mWrite.clear();
        return scale;
      }

    private:
      std::vector<float> mWrite;
  };

  class ScaleBook {
    public:
      inline ScaleBook() { }

      inline static ScaleBook tets() {
        auto book = ScaleBook();

        book.append(Scale::twelveTet());
        book.append(Scale::twentyFourTet());

        return book;
      }

      inline static ScaleBook pentatonics() {
        auto book = ScaleBook();

        book.append(Scale::pentatonicMajor());
        book.append(Scale::pentatonicMinor());

        return book;
      }

      inline static ScaleBook minors() {
        auto book = ScaleBook();

        book.append(Scale::naturalMinor());
        book.append(Scale::harmonicMinor());

        return book;
      }

      inline static ScaleBook modes() {
        auto book = ScaleBook();

        book.append(Scale::ionian());
        book.append(Scale::dorian());
        book.append(Scale::phrygian());
        book.append(Scale::lydian());
        book.append(Scale::mixolydian());
        book.append(Scale::aeolian());
        book.append(Scale::locrian());

        return book;
      }

      inline static ScaleBook all() {
        auto book = ScaleBook();

        book.append(tets());
        book.append(pentatonics());
        book.append(minors());
        book.append(modes());

        return book;
      }

      inline int          page()      const { return mPage; }
      inline const Scale& read()      const { return atPage(mPage); }
      inline const Scale& find(int i) const { return atPage(toPage(i)); }
      inline int          size()      const { return 1 + mTail.size(); }

      inline int  openTo(int i) {
        return mPage = toPage(i);
      }

      inline void append(Scale scale)      { mTail.push_back(scale); }
      inline void append(ScaleBook& book)  { appendAll(book.mTail); }
      inline void append(ScaleBook&& book) { appendAll(book.mTail); }

    private:
      inline int          toPage(int i) const { return i % size(); }
      inline const Scale& atPage(int p) const { return p == 0 ? mHead : mTail.at(p); }

      inline void appendAll(std::vector<Scale> &scales) {
        mTail.insert(mTail.end(), scales.begin(), scales.end());
      }

      int mPage = 0;

      Scale mHead = Scale::off();
      std::vector<Scale> mTail;
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
          auto centsUp   = scale.centValueAt(note);
          auto centsDown = scale.centValueAt(total - 1 - note);

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
      inline ScaleQuantizer(ScaleBook& book) : mBook(book) {
        refresh();
      }

      inline void refresh() {
        if (mConfiguredPage == mBook.page()) return;
        mConfiguredPage = mBook.page();

        auto scale = mBook.read();
        mDisabled = scale.isEmpty();
        if (mDisabled) return;

        mQuantizer.configure(scale);
        mBounds = v2d::of(0);
      }

      inline float detectedCentValue()   const { return vgetq_lane_f32(mDetected.cents(), 3); }
      inline int   detectedOctaveValue() const { return vgetq_lane_f32(mDetected.octave(), 3); }
      inline float quantizedCentValue()  const { return util::fmod(mCents, CENTS_PER_OCTAVE); }

      inline float quantize(float value) const {
        return mQuantizer.quantize(value);
      }

      inline float32x4_t process(float32x4_t value, uint32x4_t reset) {
        mDetected = dsp::four::Pitch::from(value);

        if (mDisabled)
          return value;

        auto allowChange = util::four::anyTrue(mHysteresis.process(reset));
        if (!allowChange)
          return mDetected.atCents(mCents).value();

        auto allInBounds = util::four::allInRange(mDetected.cents(), mBounds.x(), mBounds.y());
        if (allInBounds)
          return mDetected.atCents(mCents).value();

        float tmp[4];
        vst1q_f32(tmp, mDetected.cents());

        tmp[0] = mQuantizer.quantize(tmp[0]);
        tmp[1] = mQuantizer.quantize(tmp[1]);
        tmp[2] = mQuantizer.quantize(tmp[2]);

        auto i = mQuantizer.quantizeToIndex(tmp[3]);
        tmp[3] = mQuantizer.valueAt(i);
        mCents = tmp[3];
        mBounds = mQuantizer.boundsAt(i);

        return mDetected.atCents(vld1q_f32(tmp)).value();
      }

      inline float process(float value, uint32_t reset) {
        mDetected = dsp::four::Pitch::from(vdupq_n_f32(value));

        if (mDisabled) return value;

        if (!mNewHysteresis.process(reset))
          return vgetq_lane_f32(mDetected.atCents(mCents).value(), 0);

        auto dCents = detectedCentValue();
        if (dCents <= mBounds.y() && dCents >= mBounds.x())
          return vgetq_lane_f32(mDetected.atCents(mCents).value(), 0);

        auto i = mQuantizer.quantizeToIndex(dCents);
        auto qCents = mQuantizer.quantizeToIndex(i);
        mCents = qCents;
        mBounds = mQuantizer.boundsAt(i);
        return qCents;
      }

    private:
      int mConfiguredPage = -1;
      ScaleBook& mBook;

      bool mDisabled = true;
      Quantizer<128> mQuantizer;

      dsp::four::Pitch mDetected;
      float mCents = 0;
      v2d mBounds = v2d::of(0);
      dsp::simd::Timer mHysteresis { globalConfig.samplePeriod };
      dsp::Timer mNewHysteresis { globalConfig.samplePeriod };
  };
}