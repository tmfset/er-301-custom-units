#pragma once

#include <od/objects/Object.h>

namespace polygon {
  class Observable : public od::Object {
    public:
      Observable();
      virtual ~Observable();

      virtual bool isVoiceArmed(int i) = 0;
      virtual bool isVoiceNext(int i) = 0;
      virtual int groups() = 0;
      virtual int voices() = 0;

      virtual od::Parameter* vpoDirect(int voice) = 0;
      virtual od::Parameter* vpoOffset(int voice) = 0;

      virtual float envLevel(int voice) = 0;
  };
}