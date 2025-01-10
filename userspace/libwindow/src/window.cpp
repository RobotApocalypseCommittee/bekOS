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

#include "window/window.h"

#include <window/application.h>

window::Window::Window(Vec size, OwningBitmap front, OwningBitmap back)
    : m_application{nullptr}, m_id{}, m_size{size}, m_front(bek::move(front)), m_back(bek::move(back)) {}
void window::Window::show(Application& app) {
    if (m_application == &app) return;
    if (m_application) {
        m_application->remove_window(*this);
    }
    m_application = &app;
    app.register_window(this);
    auto front_id = app.register_surface(*this, m_front);
    auto back_id = app.register_surface(*this, m_back);
    m_surface_ids = {front_id, back_id};
}
void window::Window::unshow() {
    m_application->remove_window(*this);
    m_application = nullptr;
}

window::Window::~Window() = default;

core::expected<bek::shared_ptr<window::Window>> window::Window::create(Vec size) {
    return bek::adopt_shared(new Window(size, EXPECTED_TRY(OwningBitmap::create(size.x, size.y)), EXPECTED_TRY(OwningBitmap::create(size.x, size.y))));
}