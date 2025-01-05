// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BEKOS_WINDOW_CORE_H
#define BEKOS_WINDOW_CORE_H

#include <bek/types.h>
#include <bek/utility.h>

namespace window {

struct Vec {
  int x; int y;

  friend Vec operator+(const Vec& a, const Vec& b) {
    return {a.x + b.x, a.y + b.y};
  }
  Vec& operator+=(const Vec& v) {
    *this = *this + v;
    return *this;
  }
  friend Vec operator-(const Vec& a, const Vec& b) {
    return {a.x - b.x, a.y - b.y};
  }
  Vec& operator-=(const Vec& v) {
    *this = *this - v;
    return *this;
  }
  friend Vec operator*(const Vec& v, int a) {
    return {v.x * a, v.y * a};
  }
  bool positive() const {
    return (x >= 0) && (y >= 0);
  }
  friend bool operator==(const Vec &, const Vec &) = default;
};

struct Rect {
  Vec origin;
  Vec size;

  ALWAYS_INLINE int x() const { return origin.x; }
  ALWAYS_INLINE int y() const { return origin.y; }
  ALWAYS_INLINE int width() const { return size.x; }
  ALWAYS_INLINE int height() const { return size.y; }
  ALWAYS_INLINE int& x() { return origin.x; }
  ALWAYS_INLINE int& y() { return origin.y; }
  ALWAYS_INLINE int& width() { return size.x; }
  ALWAYS_INLINE int& height() { return size.y; }

  ALWAYS_INLINE int right() const { return (origin + size).x; }
  ALWAYS_INLINE int bottom() const { return (origin + size).y; }

  bool is_within(const Rect& r) const {
    return intersection(r) == *this;
  }

  bool overlaps(const Rect& r) const {
    auto i = intersection(r);
    return i.width || i.height;
  }

  Rect intersection(const Rect& r) const {
    Vec new_pos = {bek::max(x(), r.x()), bek::max(y(), r.y())};
    Vec new_extent = {
      bek::max(bek::min(right(), r.right()), new_pos.x),
      bek::max(bek::min(bottom(), r.bottom()), new_pos.y)
    };
    return {new_pos, new_extent - new_pos};
  }

  friend bool operator==(const Rect &, const Rect &) = default;
};

using Colour = u32;

}

#endif //BEKOS_WINDOW_CORE_H
