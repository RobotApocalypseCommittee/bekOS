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

#include "window/widgets/stack.h"
void window::Stack::paint(RenderContext& ctx, Rect actual_rect) {
    for (auto& child: children()) {
        auto child_actual_rect = child->relative_rect();
        child_actual_rect.origin += actual_rect.origin;

        if (ctx.confinement.overlaps(child_actual_rect)) {
            child->paint(ctx, child_actual_rect);
        }
    }
}
window::Vec window::Stack::do_layout(LayoutConstraints constraints) {
    bek::vector<Vec> child_sizes;
    child_sizes.reserve(children().size());
    auto remaining_space = constraints.max_size;
    auto cross_length = 0;
    bool vertical = m_direction == Direction::Down || m_direction == Direction::Up;
    for (auto& child: children()) {
        auto child_size = child->do_layout({remaining_space, {0, 0}});
        child_sizes.push_back(child_size);
        if (vertical) {
            remaining_space.y -= child_size.y;
            cross_length = bek::max(cross_length, child_size.x);
        } else {
            remaining_space.x -= child_size.x;
            cross_length = bek::max(cross_length, child_size.y);
        }
    }

    auto start_position = Vec{};
    cross_length = bek::max(cross_length, vertical ? constraints.min_size.x : constraints.min_size.y);
    auto used_main_length = vertical ? constraints.max_size.y - remaining_space.y : constraints.max_size.x - remaining_space.x;
    auto main_length = bek::max(used_main_length, vertical ? constraints.min_size.y : constraints.min_size.x);
    auto& main_start_pos = vertical ? start_position.y: start_position.x;
    bool flip = m_direction == Direction::Up || m_direction == Direction::Left;

    switch (m_main_alignment) {
        case Alignment::Start:
            main_start_pos = !flip ? 0: main_length - used_main_length;
            break;
        case Alignment::Centre:
            main_start_pos = !flip ? (main_length - used_main_length)/2: main_length - used_main_length/2;
            break;
        case Alignment::End:
            main_start_pos = !flip ? main_length - used_main_length : main_length;
            break;
    }


    auto children_list = children();
    auto size_it = child_sizes.begin();
    for (auto child_it = children_list.begin(); child_it != children_list.end(); child_it++, size_it++) {
        auto used_cross_length = vertical ? size_it->x: size_it->y;
        auto& cross_position = vertical ? start_position.x: start_position.y;
        switch (m_cross_alignment) {
            case Alignment::Start:
                cross_position = 0;
            break;
            case Alignment::Centre:
                cross_position = (cross_length - used_cross_length)/2;
            break;
            case Alignment::End:
                cross_position = cross_length - used_cross_length;
            break;
        }

        (*child_it)->set_layout({start_position, *size_it});
        switch (m_direction) {
            case Direction::Left:
                start_position.x -= size_it->x;
                break;
            case Direction::Down:
                start_position.y += size_it->y;
                break;
            case Direction::Right:
                start_position.x += size_it->x;
                break;
            case Direction::Up:
                start_position.y -= size_it->y;
                break;
        }
    }
    return vertical ? Vec{cross_length, main_length}: Vec{main_length, cross_length};
}