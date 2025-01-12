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

#include "window/application.h"

#include <window/Window.gen.h>

class window::internal::WindowServerConnection : public WindowClientRaw {
public:
    using WindowClientRaw::WindowClientRaw;
    void on_window_state_change(u32 id, window::Rect size) override;
    void on_mouse_move(window::Vec position, u32 buttons) override;
    void on_mouse_click(window::Vec position, u32 buttons) override;
    void on_keydown(u32 codepoint) override;
    void on_keyup(u32 codepoint) override;
    void on_ping() override { ping_response(); }
};

bek::shared_ptr<window::Application> window::Application::create(bek::string name) {
    return new Application(bek::move(name));
}
window::Application::~Application() = default;

void window::Application::blit_surface(Window& window, u32 id) {
    for (auto& held_win : m_windows) {
        if (held_win.window == &window) {
            m_connection->flip_window(held_win.window_id, id);
            return;
        }
    }
}
window::Application::Application(bek::string name) : m_name(bek::move(name)) {}

void window::Application::register_window(bek::shared_ptr<Window> window) {
    u32 next_id = 0;
    for (const auto& held_window : m_windows) {
        if (held_window.window == window) return;
        if (held_window.window_id >= next_id) {
            next_id = held_window.window_id + 1;
        }
    }
    m_windows.push_back({bek::move(window), next_id});
    m_connection->create_window(next_id, m_windows.back().window->m_size);
}

void window::Application::remove_window(Window& window) {
    for (const auto& held_window : m_windows) {
        if (held_window.window == &window) {
            m_windows.extract(held_window);
        }
    }
    // TODO: Remove from windowserver!
}
u32 window::Application::register_surface(Window& window, const OwningBitmap& bitmap) {
    u32 id = 0;
    for (auto& taken : m_surfaces_allocated) {
        if (!taken) {
            taken = true;
            break;
        }
        id++;
    }
    if (id == m_surfaces_allocated.size()) {
        m_surfaces_allocated.push_back(true);
    }
    m_connection->create_surface(id, bitmap);
    return id;
}