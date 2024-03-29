#pragma once

#include <util/math.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace ui {
  namespace dial {
    // A mixed-radix system.
    class Radix {
      public:
        // Create a radix from the left radix using the
        // step exactly as specified.
        //
        // The actual step size may be less than what's
        // given if it doesn't divide evenly into the
        // larger radix.
        static inline Radix exact(const Radix &left, float step) {
          int base = util::ceil(left.mStep / step);
          return Radix { step, base, step };
        }

        // Create a radix from the left radix using the
        // step proportionally.
        //
        // The actual step size will maintain the
        // proportion to the larger radix but will only
        // move as specified when it divides evenly.
        static inline Radix proportional(const Radix &left, float step) {
          int base = util::ceil(left.mStep / step);
          return Radix { step, base, left.mWeight / base };
        }

        // Specify the base and step size directly.
        //
        // Useful for defining the absolute largest radix
        // in order to chain the rest together.
        static inline Radix direct(int base, float step) {
          return Radix { step, base, step };
        }

        inline int   base()   const { return mBase; }
        inline float weight() const { return mWeight; }

        inline int min() const { return 0; }
        inline int max() const { return mBase - 1; }

        inline float scale(int digit) const {
          return digit * mWeight;
        }

        inline int portion(float value) const {
          return value / mWeight;
        }

      private:
        inline Radix(float step, int base, float weight) :
          mStep(step),
          mBase(base),
          mWeight(weight) { }

        inline float degree() const {
          return mBase * mWeight;
        }

        float mStep;   // Desired step size.
        int   mBase;   // Numeric base.
        float mWeight; // Actual step size.
    };

    struct Position {
      static Position min() { return Position(0, 0, 0, 0, 0); }
      static Position max() { return Position(1, 0, 0, 0, 0); }

      inline Position() :
        Position(0) { }

      inline Position(int p) :
        Position(p, p, p, p, p) { }

      inline Position(int u, int sc, int c, int f, int sf) :
        ultimate(u),
        superCoarse(sc),
        coarse(c),
        fine(f),
        superFine(sf) { }

      inline void print() {
        logRaw("p %d %d %d %d %d\n", ultimate, superCoarse, coarse, fine, superFine);
      }

      inline bool operator==(const Position &p) const {
        return superCoarse == p.superCoarse &&
               coarse      == p.coarse &&
               fine        == p.fine &&
               superFine   == p.superFine;
      }

      inline bool operator<(const Position &p) const {
        if (superCoarse < p.superCoarse) return true;
        if (coarse < p.coarse) return true;
        if (fine < p.fine) return true;
        return superFine < p.superFine;
      }

      inline bool operator>=(const Position &p) const {
        return !(*this < p);
      }

      inline void quantizeSuperFine()   { }
      inline void quantizeFine()        { superFine = 0; quantizeSuperFine(); }
      inline void quantizeCoarse()      { fine      = 0; quantizeFine(); }
      inline void quantizeSuperCoarse() { coarse    = 0; quantizeCoarse(); }

      inline bool isAtMax() const { return ultimate == 1; }

      int ultimate;
      int superCoarse;
      int coarse;
      int fine;
      int superFine;
    };

    class Hysteresis {
      public:
        inline Hysteresis() :
          Hysteresis(1, 1) { }

        inline Hysteresis(int threshold, int reset) :
          mThreshold(threshold),
          mReset(reset) { }

        int process(int change) {
          mSum += change;

          if (mSum > mThreshold) {
            int result = mSum - mThreshold;
            mSum = mReset;
            return result;
          }

          if (mSum < -mThreshold) {
            int result = mSum + mThreshold;
            mSum = -mReset;
            return result;
          }

          return 0;
        }

      private:
        int mThreshold;
        int mReset;
        int mSum = 0;
    };

    class Range {
      public:
        inline Range(float min, float max) :
          mMin(min),
          mMax(max),
          mSpan(max - min) { }

        inline float min()  const { return mMin; }
        inline float max()  const { return mMax; }
        inline float span() const { return mSpan; }

        inline Radix radix() const {
          return Radix::direct(2, span());
        }

      private:
        float mMin;
        float mMax;
        float mSpan;
    };

    class RadixSet {
      public:
        inline RadixSet(
          Radix ultimate,
          Radix superCoarse,
          Radix coarse,
          Radix fine,
          Radix superfine
        ) :
          mUltimate(ultimate),
          mSuperCoarse(superCoarse),
          mCoarse(coarse),
          mFine(fine),
          mSuperFine(superfine) {
        }

      inline void print() {
        logRaw("r %d %d %d %d %d\n", mUltimate.base(), mSuperCoarse.base(), mCoarse.base(), mFine.base(), mSuperFine.base());
        logRaw("w %f %f %f %f %f\n", mUltimate.weight(), mSuperCoarse.weight(), mCoarse.weight(), mFine.weight(), mSuperFine.weight());
      }

      inline float valueAt(const Position &p) const {
        return mUltimate.scale(p.ultimate) +
               mSuperCoarse.scale(p.superCoarse) +
               mCoarse.scale(p.coarse) +
               mFine.scale(p.fine) +
               mSuperFine.scale(p.superFine);
      }

      inline Position positionAt(float value) const {
        int u  = mUltimate.portion(value);
        value -= mUltimate.scale(u);

        int sc = mSuperCoarse.portion(value);
        value -= mSuperCoarse.scale(sc);

        int c = mCoarse.portion(value);
        value -= mCoarse.scale(c);

        int f = mFine.portion(value);
        value -= mFine.scale(f);

        int sf = mSuperFine.portion(value);
        return Position { u, sc, c, f, sf };
      }

      inline bool move(Position &p, int change, bool shift, bool fine, bool wrap) const {
        if (shift) {
          if (fine) return moveFromSuperFine(p, change, wrap);
          return moveFromSuperCoarse(p, change, wrap);
        }

        if (fine) return moveFromFine(p, change, wrap);
        return moveFromCoarse(p, change, wrap);
      }

      private:
        inline bool moveFromSuperFine(Position &p, int change, bool wrap) const {
          p.quantizeSuperFine();
          return moveSuperFine(p, change, wrap);
        }

        inline bool moveSuperFine(Position &p, int change, bool wrap) const {
          bool changed = false;

          while (change > 0) {
            if (p.isAtMax()) return moveUltimate(p, 1, wrap);
            if (p.superFine + 1 < mSuperFine.base()) {
              p.superFine++;
              changed = true;
            } else if (moveFine(p, 1, wrap)) {
              p.superFine = mSuperFine.min();
              changed = true;
            }
            change--;
          }

          while (change < 0) {
            if (p.superFine > 0) {
              p.superFine--;
              changed = true;
            } else if (moveFine(p, -1, wrap)) {
              p.superFine = mSuperFine.max();
              changed = true;
            }
            change++;
          }

          return changed;
        }

        inline bool moveFromFine(Position &p, int change, bool wrap) const {
          p.quantizeFine();
          return moveFine(p, change, wrap);
        }

        inline bool moveFine(Position &p, int change, bool wrap) const {
          bool changed = false;

          while (change > 0) {
            if (p.isAtMax()) return moveUltimate(p, 1, wrap);
            if (p.fine + 1 < mFine.base()) {
              p.fine++;
              changed = true;
            } else if (moveCoarse(p, 1, wrap)) {
              p.fine = mFine.min();
              changed = true;
            }
            change--;
          }

          while (change < 0) {
            if (p.fine > 0) {
              p.fine--;
              changed = true;
            } else if (moveCoarse(p, -1, wrap)) {
              p.fine = mFine.max();
              changed = true;
            }
            change++;
          }

          return changed;
        }

        inline bool moveFromCoarse(Position &p, int change, bool wrap) const {
          p.quantizeCoarse();
          return moveCoarse(p, change, wrap);
        }

        inline bool moveCoarse(Position &p, int change, bool wrap) const {
          bool changed = false;

          while (change > 0) {
            if (p.isAtMax()) return moveUltimate(p, 1, wrap);
            if (p.coarse + 1 < mCoarse.base()) {
              p.coarse++;
              changed = true;
            } else if (moveSuperCoarse(p, 1, wrap)) {
              p.coarse = mCoarse.min();
              changed = true;
            }
            change--;
          }

          while (change < 0) {
            if (p.coarse > 0) {
              p.coarse--;
              changed = true;
            } else if (moveSuperCoarse(p, -1, wrap)) {
              p.coarse = mCoarse.max();
              changed = true;
            }
            change++;
          }

          return changed;
        }

        inline bool moveFromSuperCoarse(Position &p, int change, bool wrap) const {
          p.quantizeSuperCoarse();
          return moveSuperCoarse(p, change, wrap);
        }

        inline bool moveSuperCoarse(Position &p, int change, bool wrap) const {
          bool changed = false;

          while (change > 0) {
            if (p.isAtMax()) return moveUltimate(p, 1, wrap);
            if (p.superCoarse + 1 < mSuperCoarse.base()) {
              p.superCoarse++;
              changed = true;
            } else if (moveUltimate(p, 1, wrap)) {
              p.superCoarse = mSuperCoarse.min();
              changed = true;
            }
            change--;
          }

          while (change < 0) {
            if (p.superCoarse > 0) {
              p.superCoarse--;
              changed = true;
            } else if (moveUltimate(p, -1, wrap)) {
              p.superCoarse = mSuperCoarse.max();
              changed = true;
            }
            change++;
          }

          return changed;
        }

        inline bool moveUltimate(Position &p, int change, bool wrap) const {
          bool changed = false;

          while (change > 0) {
            if (wrap) {
              p.ultimate = mUltimate.min();
              changed = true;
            } else if (p.ultimate + 1 < mUltimate.base()) {
              p.ultimate++;
              changed = true;
            }
            change--;
          }

          while (change < 0) {
            if (wrap) {
              p.ultimate = mUltimate.min();
              changed = true;
            } else if (p.ultimate > 0) {
              p.ultimate--;
              changed = true;
            }
            change++;
          }

          return changed;
        }

        // inline bool moveSuperCoarse(Position &p, int change, bool wrap) const {
        //   int original = p.superCoarse;
        //   int updated = original + change;

        //   if (wrap) p.superCoarse = util::mod(updated, mSuperCoarse.base());
        //   else p.superCoarse = util::clamp(updated, 0, mSuperCoarse.base());

        //   return original != p.superCoarse;
        // }

        Radix mUltimate;
        Radix mSuperCoarse;
        Radix mCoarse;
        Radix mFine;
        Radix mSuperFine;
    };

    struct Steps {
      inline Steps(float sc, float c, float f, float sf) :
        superCoarse(sc),
        coarse(c),
        fine(f),
        superFine(sf) { }

      inline RadixSet proportionalRadixSet(const Range &range) const {
        auto u  = range.radix();
        auto sc = Radix::proportional(u, superCoarse);
        auto c  = Radix::proportional(sc, coarse);
        auto f  = Radix::proportional(c, fine);
        auto sf = Radix::proportional(f, superFine);
        return RadixSet { u, sc, c, f, sf };
      }

      inline RadixSet exactRadixSet(const Range &range) const {
        auto u  = range.radix();
        auto sc = Radix::exact(u, superCoarse);
        auto c  = Radix::exact(sc, coarse);
        auto f  = Radix::exact(c, fine);
        auto sf = Radix::exact(f, superFine);
        return RadixSet { u, sc, c, f, sf };
      }

      float superCoarse;
      float coarse;
      float fine;
      float superFine;
    };
  }
}
