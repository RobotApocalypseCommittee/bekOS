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

#include "window/widgets/label.h"

window::Label::Label(bek::string text, Colour colour): m_text(bek::move(text)), m_colour(colour) {}

void window::Label::set_text(bek::string text) {
    m_text = bek::move(text);
    invalidate_layout();
    update();
}

window::Vec window::Label::do_layout(LayoutConstraints constraints) {
    auto measured = measure_text(m_text.view());
    return {bek::min(measured.x, constraints.max_size.x), bek::min(measured.y, constraints.max_size.y)};
}

void window::Label::paint(RenderContext& ctx, Rect actual_rect) {
    Renderer renderer{ctx, actual_rect};
    renderer.paint_text(m_colour, m_text.view(), {{0, 0}, actual_rect.size}, TextAlignment::Left);
}
