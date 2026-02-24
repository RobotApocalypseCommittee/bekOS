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

#ifndef BEKOS_LIBWINDOW_LABEL_H
#define BEKOS_LIBWINDOW_LABEL_H
#include <bek/str.h>

#include "widget.h"

namespace window {

class Label: public Widget {
public:
    explicit Label(bek::string text, Colour colour = WHITE);

    void set_text(bek::string text);
    bek::str_view text() const { return m_text.view(); }

    Vec do_layout(LayoutConstraints constraints) override;
    void paint(RenderContext& ctx, Rect actual_rect) override;

private:
    bek::string m_text;
    Colour m_colour;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_LABEL_H
