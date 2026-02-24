/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2025-2026 Bekos Contributors
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

#include <core/syscall.h>

#include <window/Window.gen.h>

class window::internal::WindowServerConnection: public WindowClientRaw {
public:
    using WindowClientRaw::WindowClientRaw;
    void on_window_state_change(u32 id, window::Rect size) override;
    void on_mouse_move(u32 window_id, Vec position, u32 buttons) override;
    void on_mouse_click(u32 window_id, Vec position, u32 buttons) override;
    void on_keydown(u32 window_id, u32 codepoint) override;
    void on_keyup(u32 window_id, u32 codepoint) override;
    void on_ping() override { ping_response(); }
    void on_error(ErrorCode code) override;
};

void window::internal::WindowServerConnection::on_window_state_change(u32 id, window::Rect size) {
    // TODO: Handle window resize from server
}
void window::internal::WindowServerConnection::on_mouse_move(u32 window_id, Vec position, u32 buttons) {
    // TODO: Dispatch to window
}
void window::internal::WindowServerConnection::on_mouse_click(u32 window_id, Vec position, u32 buttons) {
    // TODO: Dispatch to window
}
void window::internal::WindowServerConnection::on_keydown(u32 window_id, u32 codepoint) {
    // TODO: Dispatch to window
}
void window::internal::WindowServerConnection::on_keyup(u32 window_id, u32 codepoint) {
    // TODO: Dispatch to window
}
void window::internal::WindowServerConnection::on_error(ErrorCode code) {
    // TODO: Handle error from server
}

core::expected<bek::shared_ptr<window::Application>> window::Application::create(bek::string name) {
    auto fd = EXPECTED_TRY(core::syscall::interlink::connect("windowserver"_sv, 0));
    auto app = bek::adopt_shared(new Application(bek::move(name)));
    app->m_connection = bek::make_own<internal::WindowServerConnection>(fd);
    return app;
}
core::expected<int> window::Application::main_loop() {
    should_quit = false;
    while (!should_quit) {
        EXPECT_SUCCESS(m_connection->poll());
        for (auto& win : m_windows) {
            if (win.relayout_scheduled) {
                win.window->relayout();
                win.relayout_scheduled = false;
            }
            if (win.repaint_scheduled) {
                win.window->paint_and_flip();
                win.repaint_scheduled = false;
            }
        }
    }
    return 0;
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
window::Application::WindowData& window::Application::window_data(const Window& win) {
    for (auto& held_win : m_windows) {
        if (held_win.window == &win) {
            return held_win;
        }
    }
    ASSERT_UNREACHABLE();
}
window::Application::Application(bek::string name): m_name(bek::move(name)) {}

void window::Application::register_window(bek::shared_ptr<Window> window) {
    u32 next_id = 0;
    for (const auto& held_window : m_windows) {
        if (held_window.window == window) return;
        if (held_window.window_id >= next_id) {
            next_id = held_window.window_id + 1;
        }
    }
    m_windows.push_back({bek::move(window), next_id, true, true});
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
void window::Application::schedule_repaint(Window& window) {
    auto& data = window_data(window);
    data.repaint_scheduled = true;
}
void window::Application::schedule_relayout(Window& window) {
    auto& data = window_data(window);
    data.relayout_scheduled = true;
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
void window::Application::reregister_surface(Window& window, u32 id, OwningBitmap& bitmap) {
    m_connection->reconfigure_surface(id, {static_cast<int>(bitmap.width()), static_cast<int>(bitmap.height())},
                                      bitmap.stride());
}