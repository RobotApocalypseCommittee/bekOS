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

#ifndef BEKOS_LIBWINDOW_WIDGET_H
#define BEKOS_LIBWINDOW_WIDGET_H

#include <bek/intrusive_shared_ptr.h>
#include <bek/span.h>
#include <bek/vector.h>
#include <window/core.h>
#include <window/events.h>
#include <window/gfx.h>

namespace window {
class RootWidget;

struct LayoutConstraints {
    Vec max_size;
    Vec min_size;
};


class Widget: public bek::RefCounted<Widget> {
public:
    virtual ~Widget() = default;

    // Hierarchy
    Widget* parent() const {
        VERIFY(m_parent);
        return m_parent;
    }

    virtual bool is_root() const { return false; }

    void set_parent(Widget& parent) {
        VERIFY(!m_parent);
        m_parent = &parent;
    }

    // Children
    virtual bek::span<bek::shared_ptr<Widget>> children() { return {}; }

    Widget& hit_test(Vec position, Vec* position_in_widget = nullptr);

    // Layout
    Rect relative_rect() const { return m_relative_rect; }
    virtual Vec do_layout(LayoutConstraints constraints) = 0;
    void set_layout(Rect relative_rect) { m_relative_rect = relative_rect; }

    void invalidate_layout();


    // Events
    virtual bool on_mouse_up(const MouseEvent& event);
    virtual bool on_mouse_down(const MouseEvent& event);
    virtual bool on_mouse_move(const MouseEvent& event);
    virtual bool on_mouse_enter(const MouseEvent& event);
    virtual bool on_mouse_leave(const MouseEvent& event);

    // Painting
    virtual void paint(RenderContext& ctx, Rect actual_rect);

    void update();

protected:
    void unset_parent() { m_parent = nullptr; }
    RootWidget* get_root();

private:
    Rect m_relative_rect{};
    Widget* m_parent{};

    friend class ContainerWidget;
};



class ContainerWidget: public Widget {
public:
    bek::span<bek::shared_ptr<Widget>> children() override { return bek::span{m_children.begin(), m_children.end()}; }

    void add_widget(Widget& widget) {
        m_children.push_back({&widget});
        widget.set_parent(*this);
        on_add_widget(widget);
    }

    ~ContainerWidget() override {
        for (auto& w: m_children) {
            w->unset_parent();
        }
    }
    void remove_child(Widget& widget) {
        for (const auto& w: m_children) {
            if (w == &widget) {
                auto widget_ref = m_children.extract(w);
                widget_ref->unset_parent();
                return;
            }
        }
    }
protected:
    virtual void on_add_widget(Widget&) {}
private:
    bek::vector<bek::shared_ptr<Widget>> m_children;
};

}  // namespace window

#endif  // BEKOS_LIBWINDOW_WIDGET_H
