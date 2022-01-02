#pragma once

#include <od/graphics/text/Utils.h>
#include "util.h"
#include "interfaces.h"
#include <slew.h>
#include <vector>
#include "v2d.h"
#include <stdio.h>

using namespace common;

namespace graphics {
  struct JustifyAlign {
    inline JustifyAlign(od::Justification j, od::Alignment a) :
      justify(j), align(a) { }

    od::Justification justify;
    od::Alignment align;
  };

  #define LEFT_BOTTOM   graphics::JustifyAlign(od::justifyLeft,   od::alignBottom)
  #define LEFT_MIDDLE   graphics::JustifyAlign(od::justifyLeft,   od::alignMiddle)
  #define LEFT_TOP      graphics::JustifyAlign(od::justifyLeft,   od::alignTop)
  #define CENTER_BOTTOM graphics::JustifyAlign(od::justifyCenter, od::alignBottom)
  #define CENTER_MIDDLE graphics::JustifyAlign(od::justifyCenter, od::alignMiddle)
  #define CENTER_TOP    graphics::JustifyAlign(od::justifyCenter, od::alignTop)
  #define RIGHT_BOTTOM  graphics::JustifyAlign(od::justifyRight,  od::alignBottom)
  #define RIGHT_MIDDLE  graphics::JustifyAlign(od::justifyRight,  od::alignMiddle)
  #define RIGHT_TOP     graphics::JustifyAlign(od::justifyRight,  od::alignTop)

  struct Point {
    inline Point(float x, float y) : v(v2d::of(x, y)) { }
    inline Point(const v2d &_v) : v(_v) { }

    static inline Point of(const v2d &_v) {
      return Point(_v);
    }

    inline Point offset(const Point &by) const {
      return Point(v + by.v);
    }

    inline Point scale(float by) const {
      return Point(v * by);
    }

    inline Point offsetX(float byX) const {
      return Point(v + v2d::of(byX, 0));
    }

    inline Point offsetY(float byY) const {
      return Point(v + v2d::of(0, byY));
    }

    inline Point atX(float x) const {
      return Point(v.atX(x));
    }

    inline Point atY(float y) const {
      return Point(v.atY(y));
    }

    inline Point quantize() const {
      return Point(v.quantize());
    }

    inline void diamond(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x() - 1, v.y());
      fb.pixel(color, v.x() + 1, v.y());
      fb.pixel(color, v.x(), v.y() - 1);
      fb.pixel(color, v.x(), v.y() + 1);
    }

    inline void dot(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());
    }

    inline void arrowRight(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());

      fb.pixel(color, v.x() - 1, v.y() - 1);
      fb.pixel(color, v.x() - 2, v.y() - 2);

      fb.pixel(color, v.x() - 1, v.y() + 1);
      fb.pixel(color, v.x() - 2, v.y() + 2);
    }

    inline void arrowsRight(od::FrameBuffer &fb, od::Color color, int count, int step) const {
      v2d p = v;
      for (int i = 0; i < count; i++) {
        Point(p).arrowRight(fb, color);
        p = p + v2d::of(-step, 0);
      }
    }

    inline void arrowsLeft(od::FrameBuffer &fb, od::Color color, int count, int step) const {
      v2d p = v;
      for (int i = 0; i < count; i++) {
        Point(p).arrowLeft(fb, color);
        p = p + v2d::of(step, 0);
      }
    }

    inline void arrowLeft(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());

      fb.pixel(color, v.x() + 1, v.y() - 1);
      fb.pixel(color, v.x() + 2, v.y() - 2);

      fb.pixel(color, v.x() + 1, v.y() + 1);
      fb.pixel(color, v.x() + 2, v.y() + 2);
    }

    const v2d v;
  };

  class Range {
    public:
      static inline Range from(float left, float right) {
        return _from(util::fmin(left, right), util::fmax(left, right));
      }

      static inline Range fromLeft(float left, float width) {
        return _fromLeft(left, util::fmax(0, width));
      }
      inline Range fromLeft(float width) const {
        return fromLeft(left(), width);
      }

      static inline Range fromRight(float right, float width) {
        return _fromRight(right, util::fmax(0, width));
      }
      inline Range fromRight(float width) const {
        return fromRight(right(), width);
      }

      static inline Range fromSide(float side, float width) {
        return width > 0 ? _fromLeft(side, width) : _fromRight(side, width);
      }

      static inline Range fromCenterSpan(float center, float span) {
        return _fromCenterSpan(center, util::fabs(span));
      }
      static inline Range fromCenterWidth(float center, float width) {
        return _fromCenterWidth(center, util::fabs(width));
      }

      inline Range atCenter(float c) const {
        return _fromCenterSpan(c, halfWidth());
      }
      inline Range atCenter(const Range &other) const {
        return atCenter(other.center());
      }

      inline float clamp(float v) const {
        return util::fclamp(v, left(), right());
      }
      inline Range clamp(const Range &other) const {
        return _from(clamp(other.left()), clamp(other.right()));
      }
      inline Range clampLeft(const Range &other) const {
        return _fromLeft(clamp(other.left()), other.width());
      }
      inline Range clampRight(const Range &other) const {
        return _fromRight(clamp(other.right()), other.width());
      }

      inline Range insert(const Range &other) const {
        return _fromLeft(util::max(0, width() - other.width()))
          .clampLeft(other)
          .fromLeft(util::fmin(width(), other.width()));
      }

      inline bool contains(float v) const {
        return v >= left() && v <= right();
      }

      inline float segment(float parts, float pad) const {
        float remaining = util::max(0, width() - (parts - 1) * pad);
        return util::fdr(remaining / (float)parts);
      }

      static inline Range fromSegment(float parts, float pad, float segment) {
        return Range { 0, segment * parts + (parts - 1) * pad };
      }

      inline float left()      const { return mLeft; }
      inline float right()     const { return mRight; }
      inline float width()     const { return mWidth; }
      inline float halfWidth() const { return mHalfWidth; }
      inline float center()    const { return left() + halfWidth(); }

    private:
      static inline Range _from(float left, float right) {
        return Range { left, right };
      }

      static inline Range _fromLeft(float left, float width) {
        return Range { left, left + width };
      }
      inline Range _fromLeft(float width) const {
        return _fromLeft(left(), width);
      }

      static inline Range _fromRight(float right, float width) {
        return Range { right - width, right };
      }
      inline Range _fromRight(float width) const {
        return _fromRight(right(), width);
      }

      static inline Range _fromCenterSpan(float center, float span)   {
        return Range { center - span, center + span };
      }
      static inline Range _fromCenterWidth(float center, float width) {
        return _fromCenterSpan(center, width / 2.0f);
      }

      inline Range(float left, float right) :
        mLeft(left),
        mRight(right),
        mWidth(mRight - mLeft),
        mHalfWidth(mWidth / 2.0f) { }

      float mLeft = 0;
      float mRight = 0;
      float mWidth = 0;
      float mHalfWidth = 0;
  };

  class Line {
    public:
      inline Line(const v2d &f, const v2d &t) :
        from(f), to(t) { }

      static inline Line of(const v2d &f, const v2d &t) { return Line(f, t); }

      inline Line retreat(const v2d &f) const { return of(f, from); }
      inline Line advance(const v2d &t) const { return of(to, t); }

      inline void trace(od::FrameBuffer &fb, od::Color color) const {
        fb.line(color, from.x(), from.y(), to.x(), to.y());
      }

    private:
      v2d from;
      v2d to;
  };

  struct Circle {
    inline Circle(const v2d &c, float r) :
      center(c),
      radius(r) { }

    static inline Circle cr(const v2d &c, float r) {
      return Circle(c, r);
    }

    inline Circle offset(const v2d &by) const {
      return cr(center + by, radius);
    }

    inline Circle scale(float by) const {
      return cr(center, radius * by);
    }

    inline Circle grow(float by) const {
      return cr(center, radius + by);
    }

    inline Circle quantize() const {
      return cr(center.quantize(), radius);
    }

    inline v2d pointAtTheta(float theta) const {
      return center + v2d::ar(theta, radius);
    }

    inline void trace(od::FrameBuffer &fb, od::Color color) const {
      fb.circle(color, center.x(), center.y(), radius);
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) const {
      fb.fillCircle(color, center.x(), center.y(), radius);
    }

    const v2d center;
    const float radius;
  };

  class Box {
    public:
      static inline Box lbrt(const v2d &lb, const v2d &rt) { return _lbrt(lb.min(rt), lb.max(rt)); }
      static inline Box ltrb(const v2d &lt, const v2d &rb) { return lbrt(lt.atY(rb), rb.atY(lt)); }
      static inline Box lbwh(const v2d &lb, const v2d &wh) { return lbrt(lb, lb + wh); }
      static inline Box rbwh(const v2d &rb, const v2d &wh) { return lbwh(rb.offsetX(-wh), wh); }
      static inline Box cbwh(const v2d &cb, const v2d &wh) { return lbwh(cb.offsetX(-wh / 2.0f), wh); }
      static inline Box  cwh(const v2d &c,  const v2d &wh) { return _cwh(c, wh.abs()); }
      static inline Box   cs(const v2d &c,  float s)       { return cwh(c, v2d::of(s)); }
      static inline Box   wh(const v2d &wh)                { return lbwh(v2d::zero(), wh); }

      inline Box pad(const v2d &m)            const { return lbrt(leftBottom() + m, rightTop() - m); }
      inline Box pad(float x, float y)        const { return pad(v2d::of(x, y)); }
      inline Box pad(float m)                 const { return pad(v2d::of(m)); }
      inline Box padX(float x)                const { return pad(v2d::of(x, 0)); }
      inline Box padY(float y)                const { return pad(v2d::of(0, y)); }
      inline Box padLeftBottom(const v2d &by) const { return lbrt(leftBottom() + by, rightTop()); }
      inline Box padRightTop(const v2d &by)   const { return lbrt(leftBottom(), rightTop() - by); }
      inline Box padLeft(float by)            const { return padLeftBottom(v2d::ofX(by)); }
      inline Box padBottom(float by)          const { return padLeftBottom(v2d::ofY(by)); }
      inline Box padRight(float by)           const { return padRightTop(v2d::ofX(by)); }
      inline Box padTop(float by)             const { return padRightTop(v2d::ofY(by)); }

      inline Box square(float size) const {
        return cs(center(), size);
      }

      inline Box topLeftCorner(float w, float h) const {
        return wh(widthHeight() * v2d::of(w, h)).justifyAlign(*this, LEFT_TOP);
      }

      inline Box withWidth(float w)  const { return cwh(center(), widthHeight().atX(w)); }
      inline Box withHeight(float h) const { return cwh(center(), widthHeight().atY(h)); }

      inline Box splitLeft(float by)   const { return _lbrt(leftBottom(), rightTop().atX(left() + width() * by)); }
      inline Box splitBottom(float by) const { return _lbrt(leftBottom(), rightTop().atY(bottom() + height() * by)); }
      inline Box splitRight(float by)  const { return _lbrt(leftBottom().atX(right() - width() * by), rightTop()); }
      inline Box splitTop(float by)    const { return _lbrt(leftBottom().atY(top() - height() * by), rightTop()); }

      inline Box splitLeftPad(float by, float pad) const { return splitLeft(by).padRight(pad); }
      inline Box splitRightPad(float by, float pad) const { return splitRight(by).padLeft(pad); }

      inline Box withWidthFromLeft(float width) const {
        return lbwh(leftBottom(), widthHeight().atX(width));
      }

      inline Box scale(const v2d &by)        const { return cwh(center(), widthHeight() * by); }
      inline Box scale(float w, float h)     const { return scale(v2d::of(w, h)); }
      inline Box scale(float by)             const { return scale(v2d::of(by)); }
      inline Box scaleWidth(float by)        const { return scale(v2d::of(by, 1)); }
      inline Box scaleHeight(float by)       const { return scale(v2d::of(1, by)); }
      inline Box scaleDiscrete(int w, int h) const { return scale(v2d::of(w, h)); }

      inline Box segmentQSquare(const v2d &by) const {
        return wh(widthHeight() * by.invert()).quantize().minSquare();
      }
      inline Box segmentQSquare(float w, float h) const {
        return segmentQSquare(v2d::of(w, h));
      }

      inline Box offset(const v2d &by)    const { return _lbrt(leftBottom() + by, rightTop() + by); }
      inline Box offset(float x, float y) const { return offset(v2d::of(x, y)); }
      inline Box offsetX(float x)         const { return offset(v2d::of(x, 0)); }
      inline Box offsetY(float y)         const { return offset(v2d::of(0, y)); }

      inline Box quantizeSize()   const { return cwh(center(), widthHeight().quantize()); }
      inline Box quantizeCenter() const { return cwh(center().quantize(), widthHeight()); }
      inline Box quantize()       const { return quantizeSize().quantizeCenter(); }

      inline Box recenter(const v2d &c)       const { return cwh(c, widthHeight()); }
      inline Box recenterOn(const Box& other) const { return recenter(other.center()); }
      inline Box recenterY(float y)           const { return recenter(center().atY(y)); }
      inline Box recenterX(float x)           const { return recenter(center().atX(x)); }

      inline float width()       const { return widthHeight().x(); }
      inline float height()      const { return widthHeight().y(); }
      inline v2d   widthHeight() const { return mWidthHeight; }
      inline v2d   center()      const { return mCenter; }
      inline float centerX()     const { return center().x(); }
      inline float centerY()     const { return center().y(); }

      inline float left()       const { return leftBottom().x(); }
      inline v2d   leftBottom() const { return mLeftBottom; }
      inline v2d   leftTop()    const { return mLeftBottom.atY(top()); }
      inline v2d   leftCenter() const { return center().atX(left()); }

      inline float bottom()       const { return leftBottom().y(); }
      inline v2d   bottomCenter() const { return center().atY(bottom()); }

      inline float right()       const { return rightTop().x(); }
      inline v2d   rightBottom() const { return rightTop().atY(bottom()); }
      inline v2d   rightTop()    const { return mRightTop; }
      inline v2d   rightCenter() const { return center().atX(right()); }

      inline float top()       const { return rightTop().y(); }
      inline v2d   topCenter() const { return center().atY(top()); }

      inline Range horizontal() const { return Range::from(left(), right()); }
      inline Range vertical()   const { return Range::from(bottom(), top()); }

      inline Box intersect(const Box& other) const {
        return lbrt(
          leftBottom().max(other.leftBottom()),
          rightTop().min(other.rightTop())
        );
      }

      inline v2d clamp(const v2d &v) const {
        return v.clamp(leftBottom(), rightTop());
      }

      inline Box insert(const Box& other) const {
        auto _lb = other.leftBottom().clamp(
          leftBottom(),
          clamp(rightTop() - other.widthHeight())
        );

        auto _rt = other.rightTop().clamp(
          clamp(_lb + other.widthHeight()),
          rightTop()
        );

        return lbrt(_lb, _rt);
      }

      inline Box atLeftBottom(const v2d &lb)    const { return _lbwh(lb, widthHeight()); }
      inline Box atLeftBottom(const Box &b)     const { return atLeftBottom(b.leftBottom());}
      inline Box atLeftBottom(float l, float b) const { return atLeftBottom(v2d::of(l, b)); }
      inline Box zeroLeftBottom()               const { return atLeftBottom(v2d::zero()); }

      inline Box atRightBottom(const v2d &rb)    const { return _rbwh(rb, widthHeight()); }
      inline Box atRightBottom(const Box &b)     const { return atRightBottom(b.rightBottom()); }
      inline Box atRightBottom(float r, float b) const { return atRightBottom(v2d::of(r, b)); }

      inline Box atLeft(float left)     const { return _lbwh(leftBottom().atX(left), widthHeight()); }
      inline Box atRight(float right)   const { return atLeft(right).offsetX(-width()); }
      inline Box atCenterX(float x)     const { return cwh(center().atX(x), widthHeight()); }
      inline Box atBottom(float bottom) const { return _lbwh(leftBottom().atY(bottom), widthHeight()); }
      inline Box atTop(float top)       const { return atBottom(top).offsetY(-height()); }
      inline Box atCenterY(float y)     const { return cwh(center().atY(y), widthHeight()); }

      inline Box alignLeft(const Box &within)    const { return atLeft(within.left()); }
      inline Box alignRight(const Box &within)   const { return atRight(within.right()); }
      inline Box alignCenterX(const Box &within) const { return atCenterX(within.centerX()); }
      inline Box alignBottom(const Box &within)  const { return atBottom(within.bottom()); }
      inline Box alignTop(const Box &within)     const { return atTop(within.top()); }
      inline Box alignCenterY(const Box &within) const { return atCenterY(within.centerY()); }

      inline float  minDimension() const { return widthHeight().minDimension(); }
      inline Box    minSquare()    const { return cs(center(), minDimension()); }
      inline Circle minCircle()    const { return Circle::cr(center(), minDimension() / 2.0f); }

      static inline Box extractFrom(const od::Rect &rect) {
        return _lbrt(v2d::of(rect.left, rect.bottom), v2d::of(rect.right, rect.top));
      }

      static inline Box extractWorld(od::Graphic &graphic) {
        return extractFrom(graphic.getWorldRect());
      }

      inline void applyTo(od::Graphic &graphic) const {
        graphic.setPosition(left(), bottom());
        graphic.setSize(width(), height());
      }

      inline Box justify(const Box &within, od::Justification j) const {
        switch (j) {
          case od::justifyLeft:   return alignLeft(within);
          case od::justifyRight:  return alignRight(within);
          case od::justifyCenter: return alignCenterX(within);
        }
      }

      inline Box justify(float x, od::Justification j) const {
        switch (j) {
          case od::justifyLeft:   return atLeft(x);
          case od::justifyRight:  return atRight(x);
          case od::justifyCenter: return atCenterX(x);
        }
      }

      inline Box align(const Box &within, od::Alignment a) const {
        switch (a) {
          case od::alignTop:    return alignTop(within);
          case od::alignBottom: return alignBottom(within);
          case od::alignMiddle: return alignCenterY(within);
        }
      }

      inline Box align(float y, od::Alignment a) const {
        switch (a) {
          case od::alignTop:    return atTop(y);
          case od::alignBottom: return atBottom(y);
          case od::alignMiddle: return atCenterY(y);
        }
      }

      inline Box justifyAlign(const Box &within, JustifyAlign ja) const {
        return justify(within, ja.justify).align(within, ja.align);
      }

      inline Box justifyAlign(const v2d &v, JustifyAlign ja) const {
        return justify(v.x(), ja.justify).align(v.y(), ja.align);
      }

      inline bool containsX(float x) const {
        return x > leftBottom().x() && x < rightTop().x();
      }

      inline void fill(od::FrameBuffer &fb, od::Color color) const {
        fb.fill(color, leftBottom().x(), leftBottom().y(), rightTop().x(), rightTop().y());
      }

      inline void clear(od::FrameBuffer &fb) const {
        fb.clear(leftBottom().x(), leftBottom().y(), rightTop().x(), rightTop().y());
      }

      inline void lineTopIn(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
        fb.hline(color, leftBottom().x() + 1, rightTop().x() - 1, rightTop().y(), dotting);
      }

      inline void lineBottomIn(od::FrameBuffer &fb, od::Color color, int dotting = 0) const {
        fb.hline(color, leftBottom().x() + 1, rightTop().x() - 1, leftBottom().y(), dotting);
      }

      inline void trace(od::FrameBuffer &fb, od::Color color) const {
        fb.box(color, leftBottom().x(), leftBottom().y(), rightTop().x(), rightTop().y());
      }

      inline void outline(od::FrameBuffer &fb, od::Color color, int by = 1) const {
        fb.box(color, leftBottom().x() - by, leftBottom().y() - by, rightTop().x() + by, rightTop().y() + by);
      }

    private:
      inline Box(const v2d &leftBottom, const v2d &rightTop) :
        mLeftBottom(leftBottom),
        mRightTop(rightTop),
        mWidthHeight(mRightTop - mLeftBottom),
        mCenter(mLeftBottom + mWidthHeight * 0.5) { }

      static inline Box _lbrt(const v2d &lb, const v2d &rt) { return Box(lb, rt); }
      static inline Box _lbwh(const v2d &lb, const v2d &wh) { return _lbrt(lb, lb + wh); }
      static inline Box _rbwh(const v2d &rb, const v2d &wh) { return _lbwh(rb.offsetX(-wh), wh); }
      static inline Box _ltrb(const v2d &lt, const v2d &rb) { return _lbrt(lt.atY(rb), rb.atY(lt)); }
      static inline Box  _cwh(const v2d &c,  const v2d &wh) { return _lbwh(c - wh / 2.0f, wh); }

      const v2d mLeftBottom;
      const v2d mRightTop;
      const v2d mWidthHeight;
      const v2d mCenter;
  };

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

  class Text {
    public:
      inline Text() { }
      inline Text(std::string str, int size) {
        update(str, size);
      }

      inline void draw(
        od::FrameBuffer &fb,
        od::Color color,
        const v2d &v,
        JustifyAlign ja,
        bool outline,
        bool clear = false
      ) const {
        auto position = Box::wh(mDimensions).justifyAlign(v, ja);
        if (clear) position.clear(fb);
        fb.text(color, position.left(), position.bottom(), mValue.c_str(), mSize);
        if (outline) position.outline(fb, WHITE, 2);
      }

      inline void update(std::string str, int size) {
        mValue = str;
        mSize = size;

        int width, height;
        od::getTextSize(mValue.c_str(), &width, &height, size);
        mDimensions = v2d::of(width, height);
      }

    private:
      std::string mValue;
      int mSize;
      v2d mDimensions;
  };

  struct IFader {
    inline IFader(float _x, float _bottom, float _top, float _width) :
      x(_x),
      span(2),
      width(_width),
      height(_top - _bottom),
      bottom(_bottom),
      center(_bottom + height / 2.0f),
      top(_top) { }

    static inline IFader cwh(const v2d &c, const v2d &wh) {
      auto wh2 = wh * 0.5;
      return IFader(c.x(), c.y() - wh2.y(), c.y() + wh2.y(), wh2.x());
    }

    static inline IFader box(const Box &b) {
      return IFader(b.centerX(), b.bottom(), b.top(), b.width());
    }

    inline void draw(od::FrameBuffer &fb, od::Color color) const {
      fb.vline(color, x, bottom, top);
      fb.hline(color, x - span, x + span, bottom);
      fb.hline(color, x - span, x + span, top);
    }

    inline float drawActual(od::FrameBuffer &fb, od::Color color, float value) const {
      auto y = center + (height / 2.0f) * value;
      fb.hline(color, x - (span + 2), x + (span + 2), y);
      return y;
    }

    inline float drawTarget(od::FrameBuffer &fb, od::Color color, float value) const {
      auto y = center + (height / 2.0f) * value;
      fb.box(color, x - (span + 1), y - 1, x + (span + 1), y + 1);
      return y;
    }

    const float x;

    const int span;
    const float width;
    const float height;

    const float bottom;
    const float center;
    const float top;
  };

  struct HFader {
    inline HFader(float _y, float _left, float _right, float _height) :
      y(_y),
      hSpan(1),
      tSpan(util::fclamp(_height / 2.0f, hSpan, hSpan + 1)),
      aSpan(util::fclamp(_height / 2.0f, tSpan, tSpan + 1)),
      width(_right - _left),
      height(_height),
      left(_left),
      center(_left + width / 2.0f),
      right(_right) { }

    static inline HFader cwh(const v2d &c, const v2d &wh) {
      auto wh2 = wh * 0.5;
      return HFader(c.y(), c.x() - wh2.x(), c.x() + wh2.x(), wh2.y());
    }

    static inline HFader box(const Box &b) {
      return HFader(b.centerY(), b.left(), b.right(), b.height() / 2.0f);
    }

    inline void draw(od::FrameBuffer &fb, od::Color color) const {
      fb.hline(color, left, right, y);
      fb.vline(color, left, y - hSpan, y);
      fb.vline(color, right, y, y + hSpan);
    }

    inline float drawActual(od::FrameBuffer &fb, od::Color color, float value) const {
      auto x = util::fhr(center + (width / 2.0f) * value);
      fb.vline(color, x, y - aSpan, y + aSpan);
      return y;
    }

    inline float drawTarget(od::FrameBuffer &fb, od::Color color, float value) const {
      auto x = util::fhr(center + (width / 2.0f) * value);
      fb.box(color, x - 1, y - tSpan, x + 1, y + tSpan);
      return x;
    }

    const float y;

    const int hSpan;
    const int tSpan;
    const int aSpan;

    const float width;
    const float height;

    const float left;
    const float center;
    const float right;
  };

  struct Keyboard {
    //  1 3   6 8 A
    // 0 2 4 5 7 9 B
    static inline bool isBlackKey(int key) {
      if (key == 1) return true;
      if (key == 3) return true;
      if (key == 6) return true;
      if (key == 8) return true;
      if (key == 10) return true;
      return false;
    }

    static inline v2d keyPosition(const v2d &runRise, float key) {
      float offset = key > 4.0f ? 1.0f : 0.0f;
      return runRise * v2d::of(key + offset, isBlackKey(util::fhr(key)) ? 1.0f : 0.0f);
    }

    static inline v2d keyPositionInt(const v2d &runRise, int key) {
      float offset = key > 4.0f ? 1.0f : 0.0f;
      return runRise * v2d::of((float)key + offset, isBlackKey(key) ? 1.0f : 0.0f);
    }

    static inline v2d keyPositionInterpolate(const v2d &runRise, float key) {
      // 2.3  -> 2, 3
      // 10.2 -> 10, 11
      // 11.2 -> 11, 12
      // 11.6 -> -1, 0
      key = key > 11.5 ? key - 12.0f : key;
      auto keyLow = util::fdr(key);
      auto keyHigh = util::fur(key);
      auto low = keyPositionInt(runRise, keyLow);
      auto high = keyPositionInt(runRise, keyHigh);
      return low.lerp(high, key - keyLow);
    }

    static Circle scale(const Circle &base, float value) {
      float amount = util::flerpi(0.7f, 1.0f, util::fabs(value - (util::fdr(value) + 0.5)) * 2.0f);
      return base.scale(amount);
    }

    static inline void draw(
      od::FrameBuffer &fb,
      common::HasScale &data,
      const Circle &base,
      const v2d &runRise
    ) {
      auto size      = data.getScaleSize();
      auto isEmpty   = size == 0;

      auto detected  = data.getDetectedCentValue() / 100.0f;
      base.offset(keyPositionInterpolate(runRise, detected)).fill(fb, GRAY3);

      if (!isEmpty) {
        auto quantized = data.getQuantizedCentValue() / 100.0f;
        scale(base, quantized).offset(keyPositionInterpolate(runRise, quantized)).fill(fb, GRAY10);
      }

      for (int i = 0; i < 12; i++) {
        base.offset(keyPositionInterpolate(runRise, i)).trace(fb, GRAY5);
      }

      for (int i = 0; i < size; i++) {
        float key = data.getScaleCentValue(i) / 100.0f;
        scale(base, key).offset(keyPositionInterpolate(runRise, key)).trace(fb, GRAY13);
      }
    }
  };

  struct HKeyboard {
    inline HKeyboard(common::HasScale &data) :
        mScaleData(data) {
      mScaleData.attach();
    }

    inline ~HKeyboard() {
      mScaleData.release();
    }

    inline void draw(od::FrameBuffer &fb, const Box &world, float scale) const {
      auto inner   = world.padY(2);

      auto key     = inner.segmentQSquare(7, 2);
      auto runRise = key.widthHeight().scaleX(0.5f).quantize();

      auto bounds = Box::wh(runRise.scale(13, 2)).recenterOn(world);
      auto base   = key.atLeftBottom(bounds).minCircle().scale(scale);

      Keyboard::draw(fb, mScaleData, base, runRise);

      auto octave = mScaleData.getDetectedOctaveValue();
      //auto offset = runRise.atY(0).scale(1.5).rotateCW();
      auto arrowBase = base.center.atY(world.bottom()).offsetY(-3);
      if (octave < 0) Point(arrowBase.offset(Keyboard::keyPosition(runRise, 0))).arrowsLeft(fb, GRAY10, -octave, 3);
      if (octave > 0) Point(arrowBase.offset(Keyboard::keyPosition(runRise, 11))).arrowsRight(fb, GRAY10, octave, 3);

      auto size = mScaleData.getScaleSize();
      auto color = size == 0 ? GRAY5 : GRAY13;

      Text(mScaleData.getScaleName(), 8)
        .draw(fb, color, world.bottomCenter(), CENTER_TOP, false, true);
    }

    common::HasScale &mScaleData;
  };

  struct IKeyboard {
    inline IKeyboard(common::HasScale &data, float pad) :
        mScaleData(data),
        mPad(pad) {
      mScaleData.attach();
    }

    inline ~IKeyboard() {
      mScaleData.release();
    }
    
    inline void draw(od::FrameBuffer &fb, const Box &world) const {
      auto key = world.scale(1.0f / 2.0f, 1.0f / 7.0f).minSquare();
      auto bounds = key.scale(2.0f, 7.0f);

      auto aligned  = key.atRightBottom(bounds);
      auto diameter = aligned.minDimension();
      auto radius   = diameter / 2.0f;

      Keyboard::draw(
        fb,
        mScaleData,
        aligned.pad(mPad).minCircle(),
        v2d::of(radius, -diameter)
      );
    }

    common::HasScale &mScaleData;
    float mPad;
  };

  class ListWindow {
    public:
      static inline ListWindow from(Range window, float itemSize, float itemPad) {
        return ListWindow { window, itemSize, itemPad, 0 };
      }

      inline ListWindow scrollTo(int index, int total) const {
        auto relative = Range::fromLeft(0, relativeEnd(total - 1));
        auto window   = mWindow.atCenter(relativeCenter(index));
        auto bounded  = relative.insert(window);
        return atWindowWithOffset(bounded.atCenter(mWindow), -bounded.left());
      }

      inline float globalStartFromLeft(int i)  const { return toGlobalFromLeft(relativeStart(i)); }
      inline float globalCenterFromLeft(int i) const { return toGlobalFromLeft(relativeCenter(i)); }
      inline float globalEndFromLeft(int i)    const { return toGlobalFromLeft(relativeEnd(i)); }

      inline float globalStartFromRight(int i)  const { return toGlobalFromRight(relativeStart(i)); }
      inline float globalCenterFromRight(int i) const { return toGlobalFromRight(relativeCenter(i)); }
      inline float globalEndFromRight(int i)    const { return toGlobalFromRight(relativeEnd(i)); }

      inline bool visible(float v) const { return mWindow.contains(v); }

    private:
      inline ListWindow(Range window, float itemSize, float itemPad, float globalOffset) :
        mWindow(window),
        mItemSize(itemSize),
        mItemPad(itemPad),
        mGlobalOffset(globalOffset) { }

      inline ListWindow atWindowWithOffset(Range window, float offset) const {
        return ListWindow { window, mItemSize, mItemPad, offset };
      }

      inline float relativeStart(int i)  const { return (mItemSize + mItemPad) * i; }
      inline float relativeCenter(int i) const { return relativeStart(i) + mItemSize / 2.0f; }
      inline float relativeEnd(int i)    const { return relativeStart(i) + mItemSize; }

      inline float globalLeft()  const { return mWindow.left() + mGlobalOffset; }
      inline float globalRight() const { return mWindow.right() - mGlobalOffset; }

      inline float toGlobalFromLeft(float v)  const { return globalLeft() + v; }
      inline float toGlobalFromRight(float v) const { return globalRight() - v; }

      Range mWindow;
      float mItemSize = 0;
      float mItemPad = 0;
      float mGlobalOffset = 0;
  };

  class HChart {
    public:
      inline HChart(common::HasChart &data) :
          mChartData(data) {
        mChartData.attach();
      }

      inline ~HChart() {
        mChartData.release();
      }

      inline void draw(od::FrameBuffer &fb, const Box& world, int width, int pad) {
        auto fit = 16;

        width = util::fdr(world.horizontal().segment(fit, pad));
        auto inner = Range::fromSegment(fit, pad, width).atCenter(world.horizontal());

        auto length  = mChartData.getChartSize();
        auto current = mChartData.getChartCurrentIndex();
        auto base    = mChartData.getChartBaseIndex();

        auto window = ListWindow::from(inner, width, pad)
          .scrollTo(current, length);

        for (int i = 0; i < length; i++) {
          auto x = window.globalCenterFromLeft(i);
          if (!window.visible(x)) continue;

          auto value = mChartData.getChartValue(i);
          auto wh = v2d::of(width, value * world.height() / 2.0f);
          auto xy = world.center().atX(x);

          auto isCurrent = i == current;
          auto isBase    = i == base;
          Box::cbwh(xy, wh).fill(fb, isCurrent ? GRAY12 : GRAY10);
          if (isBase) Box::cs(xy, width + 2).trace(fb, GRAY5);
          if (isCurrent) Box::cs(xy, width + 2).trace(fb, WHITE);
        }
      }

    private:
      common::HasChart &mChartData;
  };

  class CircleChart {
    public:
      inline CircleChart(common::HasChart &data, float size) :
          mChartData(data),
          mSize(size) {
        mChartData.attach();
      }

      inline ~CircleChart() {
        mChartData.release();
      }

      inline void draw(od::FrameBuffer &fb, const Box& world) const {
        auto length  = mChartData.getChartSize();
        auto current = mChartData.getChartCurrentIndex();

        auto outer  = world.minCircle();
        auto inner  = outer.scale(mSize);
        auto span   = (outer.radius - inner.radius) / 2.0f;
        auto center = inner.grow(span);

        inner.trace(fb, GRAY1);
        center.trace(fb, GRAY5);
        outer.trace(fb, GRAY1);

        if (length < 1) return;

        float delta = M_PI * 2.0f / (float)length;

        v2d first, prev, curr;
        for (int i = 0; i < length; i++) {
          auto next = getCirclePoint(center, span, delta, i);

          if (i == 0) first = next;
          if (i == current) curr = next;

          if (i > 0) Line::of(prev, next).trace(fb, GRAY10);
          
          prev = next;
        }

        Line::of(prev, first).trace(fb, GRAY10);

        Point::of(curr).diamond(fb, WHITE);
      }

    private:
      v2d getCirclePoint(const Circle &center, float span, float delta, int i) const {
        float amount = mChartData.getChartValue(i);
        return center.grow(span * amount).pointAtTheta(delta * (float)i);
      }

      common::HasChart &mChartData;
      float mSize;
  };

  class Readout {
    public:
      inline Readout(od::Parameter &parameter) :
          mParameter(parameter) {
        mParameter.attach();
      }

      inline ~Readout() {
        mParameter.release();
      }

      void draw(
        od::FrameBuffer &fb,
        od::Color color,
        const v2d &v,
        JustifyAlign ja,
        bool outline
      ) const {
        mText.draw(fb, color, v, ja, outline);
      }

      void update(int size) {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), "%d", (int)mParameter.value());
        std::string v = tmp;
        mText.update(v, size);
      }

    private:
      od::Parameter &mParameter;
      Text mText;
  };

  class ScaleList {
    public:
      inline ScaleList(common::HasScaleBook &data) : mData(data) {
        mData.attach();
      }

      inline ~ScaleList() {
        mData.release();
      }

      void draw(
        od::FrameBuffer &fb,
        const Box &world,
        int size
      ) {
        auto length = mData.getScaleBookSize();
        auto current = mData.getScaleBookIndex();

        auto window = ListWindow::from(world.vertical(), size, 2)
          .scrollTo(current, length);

        for (int i = 0; i < length; i++) {
          auto y = window.globalCenterFromRight(i);
          if (!window.visible(y)) continue;

          auto name = mData.getScaleName(i);
          //auto size = mData.getScaleSize(i);

          auto wh = world.widthHeight().atY(size);
          auto xy = world.leftCenter().atY(y);

          auto isCurrent = i == current;
          //auto box = Box::lw(xy, wh);
          //if (isCurrent) Box::cwh(xy, wh).trace(fb, WHITE);
          Text(name, size).draw(fb, WHITE, xy, LEFT_MIDDLE, isCurrent);
        }
      }
    private:
      common::HasScaleBook &mData;
  };
}