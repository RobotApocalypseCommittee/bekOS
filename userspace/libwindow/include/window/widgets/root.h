// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025-2026 Bekos Contributors
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

#ifndef BEKOS_LIBWINDOW_ROOT_H
#define BEKOS_LIBWINDOW_ROOT_H
#include "widget.h"

namespace window {

class Window;

class RootWidget: public Widget {
public:
    bool is_root() const final { return true; }
    void notify_relayout_needed();
    void notify_repaint_needed(Rect invalid_rect);
    Vec do_layout(LayoutConstraints constraints) override;
    void paint(RenderContext& ctx, Rect actual_rect) override;

private:
    friend class Window;
    void set_widget(bek::shared_ptr<Widget> widget);
    RootWidget(Window& window);
    Window& m_window;
    bek::shared_ptr<Widget> m_widget;
};
}  // namespace window

#endif  // BEKOS_LIBWINDOW_ROOT_H
