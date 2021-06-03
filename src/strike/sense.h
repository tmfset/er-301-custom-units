#pragma once

#include <od/objects/Option.h>

#define INPUT_SENSE_LOW  1
#define INPUT_SENSE_HIGH 2

#define SENSE_LOW  0.1f
#define SENSE_HIGH 0.0f

namespace strike {
  inline float getSense(od::Option &sense) {
    switch (sense.value()) {
      case INPUT_SENSE_LOW:  return SENSE_LOW;
      case INPUT_SENSE_HIGH: return SENSE_HIGH;
      default:               return SENSE_HIGH;
    }
  }
}
