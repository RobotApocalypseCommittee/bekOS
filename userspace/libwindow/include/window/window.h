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

namespace window {

class Application;

class Window final : public bek::RefCounted<Window> {
public:
    static core::expected<bek::shared_ptr<Window>> create(Vec size);
    Window(Vec size, OwningBitmap front, OwningBitmap back);
    void show(Application& app);
    void unshow();
    Rect paint();
    ~Window();

private:
    bek::shared_ptr<Application> m_application;
    bek::optional<u32> m_id;
    Vec m_size;
    OwningBitmap m_front;
    OwningBitmap m_back;
    bek::pair<u32, u32> m_surface_ids{};
    friend class Application;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_WINDOW_H
