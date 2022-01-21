#pragma once

#include <od/objects/Followable.h>
#include <string>

namespace dsp {
  namespace objects {
    class CircularParameter : public od::Followable {
      public:
        CircularParameter(const std::string &name, float initialValue = 0.0f);
        virtual ~CircularParameter();

        void tie(od::Followable &leader);
        void untie();
        bool isTied();

        virtual float value();
        virtual float target();
        virtual int roundValue();
        virtual int roundTarget();

        void hardSet(float x);
        void softSet(float x);

        void hold();
        void unhold();

        const std::string &name();
        void setName(const std::string &name);

        bool isSerializationNeeded();
        void enableSerialization();
        void disableSerialization();
        void deserializeWithHardSet();
        void deserializeWithSoftSet();
        void deserialize(float x);

        void enableDecibelMorph();

#ifndef SWIGLUA
        CircularParameter &operator=(CircularParameter &param);
        void update();
        void forcedUpdate();
        bool offTarget();
#endif

      private:
        std::string mName;

        od::Followable *mpLeader = 0;

        float mTarget;
        float mValue;
        float mStep = 0.0f;

        int mCount = 0;
        bool mHeld = false;

        bool mDeserializeWithHardSet = false;
        bool mIsSerializationEnabled = false;
        bool mEnableDecibelMorph = false;

        void rampTo(float x);
    };
  }
}