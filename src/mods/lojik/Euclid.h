#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <vector>
#include <util.h>
#include <OneTime.h>
#include <sense.h>

#define EUCLID_MODE_TRIGGER 1
#define EUCLID_MODE_GATE 2
#define EUCLID_MODE_PASSTHROUGH 3

namespace lojik {
  class Euclid : public od::Object {
    public:
      Euclid(int max) {
        addInput(mClock);
        addInput(mReset);
        addInput(mRotate);
        addOutput(mOut);

        addParameter(mBeats);
        addParameter(mLength);
        addParameter(mShift);

        addOption(mSync);
        addOption(mMode);
        addOption(mSense);
        mMax = max;
      }

      virtual ~Euclid() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        setRythm(mBeats.value(), mLength.value());

        float *clock  = mClock.buffer();
        float *reset  = mReset.buffer();
        float *rotate = mRotate.buffer();
        float *out    = mOut.buffer();

        OneTime clockSwitch  { mClockSwitch,  false };
        OneTime rotateSwitch { mRotateSwitch, mSync.getFlag(0) };
        OneTime resetSwitch  { mResetSwitch,  mSync.getFlag(1) };
        OneTime trigSwitch   { mTrigSwitch,   false };

        int step  = mStep;
        int shift = mShift.value();
        int mode  = mMode.value();

        int startShift = shift;

        float32x4_t sense = vdupq_n_f32(getSense(mSense));
        float32x4_t zero = vdupq_n_f32(0.0f);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          float32x4_t loadClock  = vld1q_f32(clock + i);
          float32x4_t loadReset  = vld1q_f32(reset + i);
          float32x4_t loadRotate = vld1q_f32(rotate + i);

          uint32_t isClockHigh[4], isResetHigh[4], isRotateHigh[4];
          vst1q_u32(isClockHigh,  vcgtq_f32(loadClock,  sense));
          vst1q_u32(isResetHigh,  vcgtq_f32(loadReset,  zero));
          vst1q_u32(isRotateHigh, vcgtq_f32(loadRotate, zero));

          for (int j = 0; j < 4; j++) {
            clockSwitch.mark(isClockHigh[j]);
            bool c = clockSwitch.read();

            rotateSwitch.mark(isRotateHigh[j], c);
            resetSwitch.mark(isResetHigh[j], c);

            bool doRotate = rotateSwitch.read(c);
            bool doReset  = resetSwitch.read(c);
            bool doStep   = c && !doRotate;

            if (doStep)   step  = index(step, 1);
            if (doRotate) shift = index(shift, 1);
            if (doReset)  step  = 0;

            bool moved = doRotate || doReset || doStep;
            bool isHigh = mRythm[index(step, -shift)];
            float value = 0.0f;
            if (isHigh) {
              switch (mode) {
                case EUCLID_MODE_TRIGGER:
                  trigSwitch.mark(isHigh, moved);
                  value = trigSwitch.read() ? 1.0f : 0.0f;
                  break;

                case EUCLID_MODE_GATE:
                  value = 1.0f;
                  break;

                case EUCLID_MODE_PASSTHROUGH:
                  value = loadClock[j];
                  break;
              }
            }

            out[i + j] = value;
          }
        }

        mStep = step;
        int deltaShift = shift - startShift;
        if (deltaShift != 0) {
          mShift.hardSet(index(mShift.value(), deltaShift));
        }

        mClockSwitch  = clockSwitch;
        mTrigSwitch   = trigSwitch;
        mResetSwitch  = resetSwitch;
        mRotateSwitch = rotateSwitch;
      }

      od::Inlet  mClock  { "Clock" };
      od::Inlet  mReset  { "Reset" };
      od::Inlet  mRotate { "Rotate" };
      od::Outlet mOut    { "Out" };

      od::Parameter mBeats  { "Beats", 5 };
      od::Parameter mLength { "Length", 13 };
      od::Parameter mShift  { "Shift", 0 };

      od::Option mSync { "Sync", 0b00 };
      od::Option mMode { "Mode", EUCLID_MODE_TRIGGER };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

      int getStep() { return mStep; }
      int getShift() { return mShift.value(); }

      void setStep(int v) { mStep = v; }

      bool isSet(int i) { return mRythm[index(i, -getShift())]; }

    private:
      int mMax = 64;
      int mStep = 0;

      OneTime mClockSwitch;
      OneTime mTrigSwitch;
      OneTime mResetSwitch;
      OneTime mRotateSwitch;

      std::vector<bool> mRythm;
      int mCachedBeats = -1;
      bool mRegenerate = true;

      int index(int base, int offset) { return mod(base + offset, mRythm.size()); }
      int current() { return index(mStep, getShift()); }

      void setRythm(int beats, int length) {
        int cBeats  = clamp(beats, 0, mMax);
        int cLength = clamp(length, 1, mMax);

        if (mCachedBeats  != cBeats) mRegenerate = true;
        if (mRythm.size() != (size_t)cLength) mRegenerate = true;

        if (mRegenerate) {
          mRythm = generate(cBeats, cLength);
          mCachedBeats = cBeats;
          mRegenerate = false;
        }
      }

      std::vector<bool> generate(int beats, int length) {
        using namespace std;

        vector<vector<bool>> placed, remain;

        // Set up our initial values.
        for (int i = 0; i < length; i++) {
          bool init = i < beats;
          vector<bool> v = { init };

          if (init) placed.push_back(v);
          else      remain.push_back(v);
        }

        // Run that algo, you know the one:
        // https://medium.com/code-music-noise/euclidean-rhythms-391d879494df
        while (placed.size() > 1 && remain.size() > 1) {

          auto limit = min(placed.size(), remain.size());
          for (size_t i = 0; i < limit; i++) {
            placed[i].insert(placed[i].end(), remain[i].begin(), remain[i].end());
          }

          auto np = vector<vector<bool>>();
          np.insert(np.end(), placed.begin(), placed.begin() + limit);

          auto nr = vector<vector<bool>>();
          nr.insert(nr.end(), placed.begin() + limit, placed.end());
          nr.insert(nr.end(), remain.begin() + limit, remain.end());

          placed = np;
          remain = nr;
        }

        // Combine our output.
        std::vector<bool> o;
        for (size_t i = 0; i < placed.size(); i++)
          o.insert(o.end(), placed[i].begin(), placed[i].end());
        for (size_t i = 0; i < remain.size(); i++)
          o.insert(o.end(), remain[i].begin(), remain[i].end());

        return o;
      }
  };
}