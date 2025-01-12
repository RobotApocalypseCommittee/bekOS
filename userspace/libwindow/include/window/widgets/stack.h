/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2025 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_LIBWINDOW_WIDGETS_STACK_H
#define BEKOS_LIBWINDOW_WIDGETS_STACK_H
#include <bek/intrusive_shared_ptr.h>

#include "widget.h"

namespace window {

class Stack : public Widget {
public:
    enum class Direction { Left, Down, Right, Up };
    enum class Alignment { Start, Centre, End };
    Stack(Direction direction, Alignment main_alignment, Alignment cross_alignment)
        : Widget(), m_direction(direction), m_main_alignment(main_alignment), m_cross_alignment(cross_alignment) {}

    void add_child(bek::shared_ptr<Widget> child);
    void remove_child(Widget& child);

private:
    bek::vector<bek::shared_ptr<Widget>> m_children;
    Direction m_direction;
    Alignment m_main_alignment;
    Alignment m_cross_alignment;
};
}  // namespace window

#endif  // BEKOS_LIBWINDOW_WIDGETS_STACK_H
