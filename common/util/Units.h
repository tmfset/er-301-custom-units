#pragma once

#include <od/constants.h>
#include <string>
#include <util/math.h>
#include <stdio.h>

namespace util {
  enum Units {
    unitNone,
    unitDecibels,
    unitHertz,
    unitCents,
    unitMultiplier,
    unitSecs,
    unitBeats,
    unitSamples,
    unitPercent
  };

  inline void quantityToString(
    std::string &result,
    float value,
    Units units,
    int precision,
    bool suppressTrailingZeros
  ) {
    float magnitude = fabs(value);
    const char *suffix;

    switch (units) {
      case unitDecibels:
        suffix = "dB";
        precision = min(2, precision);
        break;

      case unitHertz:
        suffix = "Hz";
        break;

      case unitCents:
        suffix = "\xa2"; // ¢
        break;

      case unitMultiplier:
        suffix = "x";
        break;

      case unitSecs:
        if (magnitude == 0) {
          suffix = "s";
        } else if (magnitude < 0.1f - EPSILON) {
          suffix = "ms";
          value *= 1000;
          magnitude *= 1000;
          precision = max(0, precision - 3);
        } else {
          suffix = "s";
        }
        break;

      case unitPercent:
        suffix = "%";
        break;

      case unitNone:
      default:
        suffix = "";
        break;
    }

    if      (magnitude > 10000 - EPSILON) precision = 0;
    else if (magnitude > 1000 - EPSILON)  precision = min(1, precision);
    else if (magnitude > 100 - EPSILON)   precision = min(2, precision);

    char tmp[16];

    switch (precision) {
      case 0:
        snprintf(tmp, sizeof(tmp), "%d%s", fhr(value), suffix);
        result = tmp;
        return;

      case 1:
        snprintf(tmp, sizeof(tmp), "%0.1f", value);
        break;

      case 2:
        snprintf(tmp, sizeof(tmp), "%0.2f", value);
        break;

      case 3:
      default:
        snprintf(tmp, sizeof(tmp), "%0.3f", value);
        break;
    }

    result = tmp;
    if (suppressTrailingZeros) {
      while (result.back() == '0') result.pop_back();
      if (result.back() == '.') result.pop_back();
    }

    result += suffix;
  }

  inline std::string formatQuantity(
    float value,
    Units units,
    int precision,
    bool suppressTrailingZeros
  ) {
    std::string result;
    quantityToString(result, value, units, precision, suppressTrailingZeros);
    return result;
  }

  inline float convertToUnits(float x, Units units) {
    switch (units) {
      case unitDecibels: return toDecibels(x);
      case unitCents:    return toCents(x);
      case unitPercent:  return toPercent(x);
      default:           return x;
    }
  }

  inline float convertFromUnits(float x, Units units) {
    switch (units) {
      case unitDecibels: return fromDecibels(x);
      case unitCents:    return fromCents(x);
      case unitPercent:  return fromPercent(x);
      default:           return x;
    }
  }
}