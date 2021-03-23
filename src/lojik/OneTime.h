#pragma once

namespace lojik {
  class OneTime {
    public:
      OneTime() {}

      OneTime(const OneTime &other) {
        this->synced    = other.synced;
        this->enabled   = other.enabled;
        this->triggered = other.triggered;
      }

      OneTime(const OneTime &other, bool synced) {
        this->synced    = synced;
        this->enabled   = other.enabled;
        this->triggered = other.triggered;
      }

      void mark(bool high, bool reset = false) {
        if (!high || reset) {
          enabled = true;
        }

        if (enabled && high) {
          triggered = true;
        }
      }

      bool read(bool clock = false) {
        bool result = enabled && triggered && (!synced || clock);

        if (result) {
          triggered = false;
          enabled   = false;
        }

        return result;
      }

    private:
      bool synced = false;
      bool enabled = true;
      bool triggered = false;
  };
}