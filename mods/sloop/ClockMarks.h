#pragma once

#include <od/audio/Slices.h>
#include <vector>
#include <util/math.h>

namespace sloop {
  #define MAX_CLOCK_MARKS 1024

  class ClockMarks {
    public:
      ClockMarks() {}
      virtual ~ClockMarks() {}

      void set(int step, int position) {
        int incSize = util::max(step + 1, size());
        marks.resize(util::clamp(incSize, 0, MAX_CLOCK_MARKS));
        if (step < size()) marks[step] = position;
      }

      int size() const {
        return marks.size();
      }

      int get(int step) const {
        if (step < 0) return 0;
        if (step < size()) return marks.at(step);
        return 0;
      }

    private:
      std::vector<int> marks;
      od::Slices *mpSlices = 0;
  };
}