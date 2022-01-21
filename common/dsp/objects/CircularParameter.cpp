#include <dsp/objects/CircularParameter.h>
#include <od/extras/Utils.h>
#include <util/math.h>

namespace dsp {
  namespace objects {
    CircularParameter::CircularParameter(
      const std::string &name,
      float initialValue
    ) : mName(name),
        mTarget(initialValue),
        mValue(initialValue) { }

    CircularParameter::~CircularParameter() {
      untie();
    }

    void CircularParameter::tie(od::Followable &leader) {
      untie();
      mpLeader = &leader;
      mpLeader->attach();
    }

    void CircularParameter::untie() {
      if (!mpLeader) return;
      mpLeader->release();
      mpLeader = 0;
    }

    bool CircularParameter::isTied() {
      return mpLeader;
    }

    float CircularParameter::value() {
      if (mpLeader) return mpLeader->value();
      return mValue;
    }

    float CircularParameter::target() {
      if (mpLeader) return mpLeader->target();
      return mTarget;
    }

    void CircularParameter::rampTo(float x) {
      // TODO: actually be circular
      float diff = x - mValue;
      if (util::fabs(diff) < 1e-10f) {
        mValue = x;
        mCount = 0;
      } else {
        mCount = 50;
        mStep = diff * 0.02f;
      }
    }

    void CircularParameter::hardSet(float x) {
      if (mpLeader) return;
      mTarget = mValue = x;
      mCount = 0;
    }

    void CircularParameter::softSet(float x) {
      if (mpLeader) return;
      mTarget = x;
      rampTo(x);
    }

    void CircularParameter::deserialize(float x) {
      if (mDeserializeWithHardSet) hardSet(x);
      else softSet(x);
    }

    void CircularParameter::hold() {
      mHeld = true;
    }

    void CircularParameter::unhold() {
      mTarget = mValue;
      mCount = 0;
      mHeld = false;
    }

    int CircularParameter::roundValue() {
      if (mpLeader) return mpLeader->roundValue();
      return od::fastRound(mValue);
    }

    int CircularParameter::roundTarget() {
      if (mpLeader) return mpLeader->roundTarget();
      return od::fastRound(mTarget);
    }

    const std::string &CircularParameter::name() {
      return mName;
    }

    void CircularParameter::setName(const std::string &name) {
      mName = name;
    }

    CircularParameter &CircularParameter::operator=(CircularParameter &other) {
      hardSet(other.target());
      return *this;
    }

    void CircularParameter::forcedUpdate() {
      if (mpLeader) return;

      if (mCount > 0) {
        mValue += mStep;
        mCount--;
      } else {
        mValue = mTarget;
      }
    }

    void CircularParameter::update() {
      if (mpLeader) return;
      if (mHeld) return;

      if (mCount > 0) {
        mValue += mStep;
        mCount--;
      } else {
        mValue = mTarget;
      }
    }

    bool CircularParameter::isSerializationNeeded() {
      return mIsSerializationEnabled;
    }

    void CircularParameter::enableSerialization() {
      mIsSerializationEnabled = true;
    }

    void CircularParameter::disableSerialization() {
      mIsSerializationEnabled = false;
    }

    void CircularParameter::deserializeWithHardSet() {
      mDeserializeWithHardSet = true;
    }

    void CircularParameter::deserializeWithSoftSet() {
      mDeserializeWithHardSet = false;
    }

    bool CircularParameter::offTarget() {
      return mCount > 0;
    }

    void CircularParameter::enableDecibelMorph() {
      mEnableDecibelMorph = true;
    }
  }
}