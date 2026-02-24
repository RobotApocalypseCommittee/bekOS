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

#ifndef BEKOS_LIBWINDOW_APPLICATION_H
#define BEKOS_LIBWINDOW_APPLICATION_H
#include <bek/intrusive_shared_ptr.h>
#include <bek/str.h>
#include <bek/vector.h>

#include "window.h"

namespace window {
namespace internal {
class WindowServerConnection;
}

class Application final: public bek::RefCounted<Application> {
public:
    static core::expected<bek::shared_ptr<Application>> create(bek::string name);

    core::expected<int> main_loop();
    ~Application();

    void quit() { should_quit = true; }

private:
    struct WindowData {
        bek::shared_ptr<Window> window;
        u32 window_id;
        bool repaint_scheduled;
        bool relayout_scheduled;
    };

    void register_window(bek::shared_ptr<Window> window);
    void remove_window(Window& window);

    void schedule_repaint(Window& window);
    void schedule_relayout(Window& window);

    u32 register_surface(Window& window, const OwningBitmap& bitmap);
    void reregister_surface(Window& window, u32 id, OwningBitmap& bitmap);

    void blit_surface(Window& window, u32 id);

    WindowData& window_data(const Window& win);

    explicit Application(bek::string name);
    bek::string m_name;
    bek::vector<WindowData> m_windows;
    bek::own_ptr<internal::WindowServerConnection> m_connection;
    bek::vector<bool> m_surfaces_allocated;
    bool should_quit{};
    friend class Window;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_APPLICATION_H
