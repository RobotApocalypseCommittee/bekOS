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

#include "window/widgets/widget.h"

window::Widget& window::Widget::hit_test(Vec position, Vec* position_in_widget) {
    auto* current_widget = this;
    auto current_position = position;
    bool should_continue = true;
    while (should_continue) {
        should_continue = false;
        for (auto& child : current_widget->children()) {
            if (child->relative_rect().contains(current_position)) {
                current_widget = child.get();
                current_position = position - child->relative_rect().origin;
                should_continue = true;
                break;
            }
        }
    }
    if (position_in_widget) *position_in_widget = current_position;
    return *current_widget;
}
void window::Widget::invalidate_layout() {
    if (auto* root = get_root()) {
        //root->notify_relayout_needed();
    }
}
void window::Widget::update() {
    auto* current_widget = this;
    auto current_rect = m_relative_rect;
    while (current_widget->parent()) {
        current_widget = current_widget->parent();
        current_rect.origin += current_widget->m_relative_rect.origin;
    }
    if (current_widget->is_root()) {
        static_cast<RootWidget*>(current_widget)->notify_repaint_needed(current_rect);
    }
}

window::RootWidget* window::Widget::get_root() {
    auto* current_widget = this;
    while (current_widget->parent()) {
        current_widget = current_widget->parent();
    }
    if (current_widget->is_root()) {
        return static_cast<RootWidget*>(current_widget);
    } else {
        return nullptr;
    }
}