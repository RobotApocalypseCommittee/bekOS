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

#include "window/widgets/window_frame.h"

#include <bek/optional.h>
#include <window/widgets/root.h>
#include <window/window.h>

namespace window {

inline constexpr Colour TITLE_BAR_FOCUSED = from_rgb(60, 60, 140);
inline constexpr Colour TITLE_BAR_UNFOCUSED = from_rgb(80, 80, 80);
inline constexpr Colour CLOSE_BUTTON_COLOUR = from_rgb(200, 50, 50);
inline constexpr Colour BORDER_COLOUR = from_rgb(100, 100, 100);
inline constexpr Colour TITLE_TEXT_COLOUR = WHITE;

inline constexpr int CLOSE_ICON_SIZE = 24;

static bek::optional<OwningBitmap>* g_close_icon = nullptr;
static bool g_close_icon_loaded = false;

static bek::optional<OwningBitmap>& close_icon() {
    if (!g_close_icon_loaded) {
        g_close_icon_loaded = true;
        g_close_icon = new bek::optional<OwningBitmap>();
        auto res = load_tga("/res/close.tga"_sv);
        if (!res.has_error()) {
            *g_close_icon = res.release_value();
        }
    }
    return *g_close_icon;
}

WindowFrame::WindowFrame(bek::string title) : m_title(bek::move(title)) {}

void WindowFrame::set_content(bek::shared_ptr<Widget> content) {
    if (m_content) {
        remove_child(*m_content);
    }
    m_content = bek::move(content);
    if (m_content) {
        add_widget(*m_content);
    }
}

Vec WindowFrame::do_layout(LayoutConstraints constraints) {
    int content_max_w = constraints.max_size.x - 2 * BORDER_THICKNESS;
    int content_max_h = constraints.max_size.y - TITLE_BAR_HEIGHT - BORDER_THICKNESS;
    if (content_max_w < 0) content_max_w = 0;
    if (content_max_h < 0) content_max_h = 0;

    if (m_content) {
        auto content_size = m_content->do_layout({{content_max_w, content_max_h}, {0, 0}});
        m_content->set_layout({{BORDER_THICKNESS, TITLE_BAR_HEIGHT}, content_size});
    }

    // Fill the entire window
    return constraints.max_size;
}

void WindowFrame::paint(RenderContext& ctx, Rect actual_rect) {
    Renderer renderer{ctx, actual_rect};

    auto* win = find_window();
    bool focused = win && win->is_focused();

    // Title bar background
    Rect title_bar{
        {0, 0},
        {actual_rect.width(), TITLE_BAR_HEIGHT},
    };
    renderer.paint_rect(focused ? TITLE_BAR_FOCUSED : TITLE_BAR_UNFOCUSED, title_bar);

    // Title text
    Rect text_region{
        {CLOSE_BUTTON_MARGIN, 0},
        {actual_rect.width() - CLOSE_BUTTON_SIZE - CLOSE_BUTTON_MARGIN, TITLE_BAR_HEIGHT},
    };
    renderer.paint_text(TITLE_TEXT_COLOUR, m_title.view(), text_region, TextAlignment::Left);

    // Close button
    Rect close_btn = close_button_rect();
    renderer.paint_rect(CLOSE_BUTTON_COLOUR, close_btn);
    auto& icon = close_icon();
    if (icon) {
        int inset = (CLOSE_BUTTON_SIZE - CLOSE_ICON_SIZE) / 2;
        Rect icon_rect{
            {close_btn.x() + inset, close_btn.y() + inset},
            {CLOSE_ICON_SIZE, CLOSE_ICON_SIZE},
        };
        renderer.paint_bitmap_with_transparency(*icon, icon_rect, {0, 0});
    }

    // Border (left, right, bottom edges)
    // Left border
    renderer.paint_rect(BORDER_COLOUR, {{0, TITLE_BAR_HEIGHT}, {BORDER_THICKNESS, actual_rect.height() - TITLE_BAR_HEIGHT}});
    // Right border
    renderer.paint_rect(BORDER_COLOUR, {{actual_rect.width() - BORDER_THICKNESS, TITLE_BAR_HEIGHT}, {BORDER_THICKNESS, actual_rect.height() - TITLE_BAR_HEIGHT}});
    // Bottom border
    renderer.paint_rect(BORDER_COLOUR, {{0, actual_rect.height() - BORDER_THICKNESS}, {actual_rect.width(), BORDER_THICKNESS}});

    // Paint content child
    if (m_content) {
        m_content->paint(ctx, {actual_rect.origin + m_content->relative_rect().origin, m_content->relative_rect().size});
    }
}

bool WindowFrame::on_mouse_down(const MouseEvent& event) {
    Rect close_btn = close_button_rect();
    if (close_btn.contains(event.location)) {
        auto* win = find_window();
        if (win) {
            win->request_close();
        }
        return true;
    }

    Rect title_bar = title_bar_rect();
    if (title_bar.contains(event.location)) {
        auto* win = find_window();
        if (win) {
            win->begin_move();
        }
        return true;
    }

    return false;
}

Rect WindowFrame::title_bar_rect() const {
    return {{0, 0}, {relative_rect().width(), TITLE_BAR_HEIGHT}};
}

Rect WindowFrame::close_button_rect() const {
    return {
        {relative_rect().width() - CLOSE_BUTTON_SIZE, 0},
        {CLOSE_BUTTON_SIZE, CLOSE_BUTTON_SIZE},
    };
}

Rect WindowFrame::content_rect() const {
    return {
        {BORDER_THICKNESS, TITLE_BAR_HEIGHT},
        {relative_rect().width() - 2 * BORDER_THICKNESS, relative_rect().height() - TITLE_BAR_HEIGHT - BORDER_THICKNESS},
    };
}

Window* WindowFrame::find_window() {
    auto* root = get_root();
    if (root) {
        return &root->window();
    }
    return nullptr;
}

}  // namespace window
