#pragma once

#include <od/objects/Object.h>

namespace polygon {
  class Observable : public od::Object {
    public:
      Observable();
      virtual ~Observable();

      virtual int groups() = 0;
      virtual int voices() = 0;

      virtual od::Parameter* vpoRoundRobin() = 0;
      virtual od::Parameter* vpoDirect(int voice) = 0;
      virtual od::Parameter* vpoOffset(int voice) = 0;

      virtual float envLevel(int voice) = 0;
  };
}