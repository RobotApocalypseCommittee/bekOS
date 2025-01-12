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

#ifndef BEKOS_LIBWINDOW_WIDGET_H
#define BEKOS_LIBWINDOW_WIDGET_H

#include <bek/intrusive_shared_ptr.h>
#include <bek/span.h>
#include <bek/vector.h>
#include <window/core.h>
#include <window/events.h>
#include <window/gfx.h>

namespace window {

struct LayoutConstraints {
    Vec max_size;
    Vec min_size;
};

class Widget: public bek::RefCounted<Widget> {
public:
    virtual ~Widget() = default;

    // Children
    virtual bek::span<bek::shared_ptr<Widget>> children();

    Widget& hit_test(Vec position, Vec* position_in_widget = nullptr);

    // Layout
    Rect relative_rect() const { return m_relative_rect; }
    virtual Vec layout_size(LayoutConstraints constraints) const;
    void set_layout(Rect relative_rect) { m_relative_rect = relative_rect; };

    // Events
    virtual bool on_mouse_up(const MouseEvent& event);
    virtual bool on_mouse_down(const MouseEvent& event);
    virtual bool on_mouse_move(const MouseEvent& event);
    virtual bool on_mouse_enter(const MouseEvent& event);
    virtual bool on_mouse_leave(const MouseEvent& event);

    // Painting
    virtual void paint(RenderContext& ctx, Rect actual_rect);

private:
    Rect m_relative_rect{};
    Widget* m_parent{};
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_WIDGET_H
