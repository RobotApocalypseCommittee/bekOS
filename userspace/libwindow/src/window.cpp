// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025-2026 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "window/window.h"

#include <window/application.h>

window::Window::Window(Vec size, OwningBitmap front, OwningBitmap back)
    : m_application{nullptr},
      m_id{},
      m_size{size},
      m_front(bek::move(front)),
      m_back(bek::move(back)),
      m_root_widget(*this) {}

void window::Window::set_content(bek::shared_ptr<Widget> widget) {
    m_root_widget.set_widget(bek::move(widget));
    if (m_application) {
        queue_relayout();
    }
}

void window::Window::show(Application& app) {
    if (m_application == &app) return;
    if (m_application) {
        m_application->remove_window(*this);
    }
    m_application = &app;
    app.register_window(this);
    auto front_id = app.register_surface(*this, m_front);
    auto back_id = app.register_surface(*this, m_back);
    m_surface_ids = {front_id, back_id};
}
void window::Window::unshow() {
    m_application->remove_window(*this);
    m_application = nullptr;
}

window::Window::~Window() = default;

void window::Window::relayout() {
    auto sz = m_root_widget.do_layout({m_size, m_size});
    m_root_widget.set_layout({{0, 0}, sz});
    m_dirty_rect = {{0, 0}, m_size};
}

window::Rect window::Window::paint_and_flip() {
    auto render_ctx = RenderContext::create(m_back.buffer(), m_back.stride(), m_back.width(), m_back.height());
    {
        auto confinement = bek::exchange(m_previous_dirty_rect, m_dirty_rect);
        auto painter = Renderer{render_ctx, confinement};
        painter.paint_bitmap(m_front, {{}, confinement.size}, confinement.origin);
    }

    render_ctx.confinement = bek::exchange(m_dirty_rect, {{0, 0}, {0, 0}});
    m_root_widget.paint(render_ctx, {{}, m_size});
    m_application->blit_surface(*this, m_surface_ids.second);
    bek::swap(m_front, m_back);
    bek::swap(m_surface_ids.first, m_surface_ids.second);
    return render_ctx.confinement;
}
void window::Window::queue_relayout() { m_application->schedule_relayout(*this); }
void window::Window::queue_repaint(Rect rect) {
    m_dirty_rect = m_dirty_rect.union_with(rect);
    m_application->schedule_repaint(*this);
}
void window::Window::on_mouse_move(MouseEvent mouse_event) {
    auto& widget = m_root_widget.hit_test(mouse_event.location);
    if (m_hovered_widget != &widget) {
        {
            bek::shared_ptr<Widget> new_hovered{&widget};
            m_hovered_widget->on_mouse_leave(mouse_event);
            m_hovered_widget = bek::move(new_hovered);
        }
        m_hovered_widget->on_mouse_enter(mouse_event);
    } else {
        m_hovered_widget->on_mouse_move(mouse_event);
    }
}

core::expected<bek::shared_ptr<window::Window>> window::Window::create(Vec size) {
    return bek::adopt_shared(new Window(size, EXPECTED_TRY(OwningBitmap::create(size.x, size.y)),
                                        EXPECTED_TRY(OwningBitmap::create(size.x, size.y))));
}

#pragma region RootWidget

void window::RootWidget::set_widget(bek::shared_ptr<Widget> widget) {
    if (m_widget) {
        m_widget->unset_parent();
    }
    m_widget = bek::move(widget);
    if (m_widget) {
        m_widget->set_parent(*this);
    }
}

void window::RootWidget::notify_relayout_needed() { m_window.queue_relayout(); }
void window::RootWidget::notify_repaint_needed(Rect invalid_rect) { m_window.queue_repaint(invalid_rect); }

window::Vec window::RootWidget::do_layout(LayoutConstraints constraints) {
    // We centre our only widget
    if (m_widget) {
        auto sz = m_widget->do_layout({constraints.max_size, {0, 0}});
        auto pos = (constraints.max_size - sz) / 2;
        m_widget->set_layout({pos, sz});
    }
    return constraints.max_size;
}

void window::RootWidget::paint(RenderContext& ctx, Rect actual_rect) {
    Renderer renderer{ctx, actual_rect};
    if (!m_widget || !ctx.confinement.is_within(m_widget->relative_rect())) {
        renderer.paint_rect(0xAAAAAAAA, actual_rect);
    }
    if (m_widget) {
        m_widget->paint(ctx, m_widget->relative_rect());
    }
}

window::RootWidget::RootWidget(Window& window): m_window(window) {}

#pragma endregion