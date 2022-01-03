#pragma once

#include <graphics/primitives/all.h>

namespace graphics {
  struct Grid {
    inline Grid(const Box &topLeft, int _cols, int _rows, float pad) :
      mark(topLeft.quantize().pad(pad)),
      cols(_cols),
      rows(_rows),
      cStep(topLeft.widthHeight().x()),
      rStep(-topLeft.widthHeight().y()) { }

    static inline Grid create(const Box& world, int cols, int rows, float pad) {
      float iCols = 1.0f / cols;
      float iRows = 1.0f / rows;

      auto corner = world.topLeftCorner(iCols, iRows).minSquare().quantizeSize();
      auto grid = corner.scaleDiscrete(cols, rows).recenterOn(world).quantizeCenter();

      return Grid(grid.topLeftCorner(iCols, iRows), cols, rows, pad);
    }

    static inline Grid createRect(const Box& world, int cols, int rows, float pad) {
      float iCols = 1.0f / cols;
      float iRows = 1.0f / rows;

      auto corner = world.topLeftCorner(iCols, iRows).quantizeSize();
      auto grid = corner.scaleDiscrete(cols, rows).recenterOn(world).quantizeCenter();

      return Grid(grid.topLeftCorner(iCols, iRows), cols, rows, pad);
    }

    inline int index(int c, int r) const {
      return c * rows + r;
    }

    inline Box cell(int c, int r) const {
      return mark.offset(cStep * c, rStep * r);
    }

    const Box mark;
    const int cols;
    const int rows;
    const float cStep;
    const float rStep;
  };
}