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

#ifndef BEKOS_LIBWINDOW_WINDOW_H
#define BEKOS_LIBWINDOW_WINDOW_H
#include <bek/intrusive_shared_ptr.h>
#include <bek/optional.h>

#include "gfx.h"
#include "widgets/root.h"

namespace window {

class Application;

class Window final : public bek::RefCounted<Window> {
public:
    static core::expected<bek::shared_ptr<Window>> create(Vec size);
    Window(Vec size, OwningBitmap front, OwningBitmap back);

    Vec size() const { return m_size; }
    void show(Application& app);
    void unshow();
    ~Window();

    void relayout();
    Rect paint_and_flip();

    void queue_relayout();
    void queue_repaint(Rect rect);

    void on_mouse_move(MouseEvent evt);

private:
    bek::shared_ptr<Application> m_application;
    bek::shared_ptr<Widget> m_hovered_widget;
    bek::optional<u32> m_id;
    Vec m_size;
    OwningBitmap m_front;
    OwningBitmap m_back;
    bek::pair<u32, u32> m_surface_ids{};
    RootWidget m_root_widget;
    Rect m_dirty_rect;
    Rect m_previous_dirty_rect;
    friend class Application;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_WINDOW_H
