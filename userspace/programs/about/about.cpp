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

#include <core/io.h>

#include <window/application.h>
#include <window/widgets/label.h>
#include <window/widgets/stack.h>
#include <window/window.h>

int main() {
    core::fprintln(core::stderr, "about: creating app"_sv);
    auto app_result = window::Application::create(bek::string("about"_sv));
    if (app_result.has_error()) {
        core::fprintln(core::stderr, "Failed to connect to windowserver: {}"_sv, app_result.error());
        return -1;
    }
    auto app = app_result.release_value();
    core::fprintln(core::stderr, "about: app created"_sv);

    core::fprintln(core::stderr, "about: creating window"_sv);
    auto win_result = window::Window::create({300, 230});
    if (win_result.has_error()) {
        core::fprintln(core::stderr, "Failed to create window: {}"_sv, win_result.error());
        return -1;
    }
    auto win = win_result.release_value();
    core::fprintln(core::stderr, "about: window created"_sv);

    core::fprintln(core::stderr, "about: creating stack"_sv);
    auto stack = bek::adopt_shared(new window::Stack(window::Stack::Direction::Down, window::Stack::Alignment::Centre,
                                                     window::Stack::Alignment::Centre));
    core::fprintln(core::stderr, "about: stack created"_sv);

    core::fprintln(core::stderr, "about: creating labels"_sv);
    auto title_label = bek::adopt_shared(new window::Label(bek::string("bekOS"_sv)));
    core::fprintln(core::stderr, "about: title label created"_sv);
    auto version_label = bek::adopt_shared(new window::Label(bek::string("Version 0.1"_sv)));
    core::fprintln(core::stderr, "about: version label created"_sv);

    stack->add_widget(*title_label);
    stack->add_widget(*version_label);
    core::fprintln(core::stderr, "about: widgets added to stack"_sv);

    win->set_decorated_content(bek::string("About bekOS"_sv), stack);
    core::fprintln(core::stderr, "about: content set"_sv);
    win->show(*app);
    core::fprintln(core::stderr, "about: window shown"_sv);

    core::fprintln(core::stderr, "about: entering main loop"_sv);
    auto result = app->main_loop();
    if (result.has_error()) {
        core::fprintln(core::stderr, "Main loop error: {}"_sv, result.error());
        return -1;
    }
    return result.value();
}
