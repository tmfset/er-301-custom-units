#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class RegisterLike : public od::Object {
    public:
      RegisterLike();
      virtual ~RegisterLike();

      virtual int length() = 0;
      virtual int current() = 0;
      virtual float value(int i) = 0;
  };
}