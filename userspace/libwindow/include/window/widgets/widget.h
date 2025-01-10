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

#ifndef BEKOS_LIBWINDOW_WIDGET_H
#define BEKOS_LIBWINDOW_WIDGET_H

#include <bek/vector.h>
#include <bek/intrusive_shared_ptr.h>
#include <window/core.h>
#include <window/events.h>
#include <window/gfx.h>

namespace window {

struct LayoutParameters {
    Vec preferred_size;
    Vec min_size;
    Vec max_size;
};

class Widget: public bek::RefCounted<Widget> {
public:
    virtual ~Widget() = default;
    virtual LayoutParameters get_layout_parameters() const;
    Rect rect() const { return m_rect; }

    virtual bool on_click(const MouseEvent& event);
    virtual bool on_mouse_move(const MouseEvent& event);


protected:
    virtual void do_layout();
    void needs_repaint();
    void paint(RenderContext& ctx);

private:
    void set_layout(Rect layout);
    Rect m_rect;
    Widget* m_parent;
    bek::vector<bek::shared_ptr<Widget>> m_children;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_WIDGET_H
