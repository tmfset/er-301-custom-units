#pragma once

#include <od/objects/Followable.h>

#include <util/Units.h>

namespace graphics {
    class FollowableValue {
    public:
      inline FollowableValue(od::Followable &followable) :
          mFollowable(followable) {
        mFollowable.attach();
      }

      inline ~FollowableValue() {
        mFollowable.release();
      }

      inline float setValueInUnits(float v, bool hard) {
        auto fromUnits = util::convertFromUnits(v, mUnits);

        if (hard) mFollowable.hardSet(fromUnits);
        else      mFollowable.softSet(fromUnits);

        return fromUnits;
      }

      inline void setShowTarget(bool v) {
        mForceRefresh = true;
        mShowTarget = v;
      }

      inline void setUnits(util::Units v) {
        mForceRefresh = true;
        mUnits = v;
      }

      inline bool refresh() {
        auto current = currentValue();

        auto isChanged = mLastValue != current;
        auto shouldRefresh = mForceRefresh || isChanged;
        if (!shouldRefresh) return false;

        mForceRefresh = false;
        mLastValue = current;
        mLastValueInUnits = util::convertToUnits(mLastValue, mUnits);

        return true;
      }

      inline bool hasMoved() {
        return mLastValue != currentValue();
      }

      inline float lastValueInUnits() {
        return mLastValueInUnits;
      }

      inline std::string toString(int precision, bool suppressZeros) {
        return util::formatQuantity(mLastValueInUnits, mUnits, precision, suppressZeros);
      }

    private:
      inline float currentValue() {
        return mShowTarget ? mFollowable.target() : mFollowable.value();
      }

      od::Followable &mFollowable;
      bool  mForceRefresh = true;
      float mLastValue = 0;
      float mLastValueInUnits = 0;

      bool mShowTarget = false;
      util::Units mUnits = util::unitNone;
  };
}