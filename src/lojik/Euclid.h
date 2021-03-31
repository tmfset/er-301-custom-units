#pragma once

#include <od/objects/Object.h>
#include <vector>
#include <util.h>
#include <OneTime.h>

#define EUCLID_MODE_TRIGGER 1
#define EUCLID_MODE_GATE 2
#define EUCLID_MODE_PASSTHROUGH 3

namespace lojik {
  class Euclid : public od::Object {
    public:
      Euclid(int max);
      virtual ~Euclid();

  #ifndef SWIGLUA
    virtual void process();

    od::Inlet  mClock  { "Clock" };
    od::Inlet  mReset  { "Reset" };
    od::Inlet  mRotate { "Rotate" };
    od::Outlet mOut    { "Out" };

    od::Parameter mBeats  { "Beats", 5 };
    od::Parameter mLength { "Length", 13 };
    od::Parameter mShift  { "Shift", 0 };

    od::Option mSync { "Sync", 0b00 };
    od::Option mMode { "Mode", EUCLID_MODE_TRIGGER };
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