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

#ifndef BEKOS_LIBWINDOW_WINDOW_FRAME_H
#define BEKOS_LIBWINDOW_WINDOW_FRAME_H

#include <bek/str.h>

#include "widget.h"

namespace window {

class Window;

class WindowFrame : public ContainerWidget {
public:
    explicit WindowFrame(bek::string title);

    void set_content(bek::shared_ptr<Widget> content);

    Vec do_layout(LayoutConstraints constraints) override;
    void paint(RenderContext& ctx, Rect actual_rect) override;
    bool on_mouse_down(const MouseEvent& event) override;

    static constexpr int TITLE_BAR_HEIGHT = 32;
    static constexpr int CLOSE_BUTTON_SIZE = TITLE_BAR_HEIGHT;
    static constexpr int CLOSE_BUTTON_MARGIN = 4;
    static constexpr int BORDER_THICKNESS = 1;

private:
    Rect title_bar_rect() const;
    Rect close_button_rect() const;
    Rect content_rect() const;

    Window* find_window();

    bek::string m_title;
    bek::shared_ptr<Widget> m_content;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_WINDOW_FRAME_H
