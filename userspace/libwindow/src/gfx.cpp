// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
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

#include "window/gfx.h"

#include <bek/allocations.h>
#include <bek/own_ptr.h>
#include <core/syscall.h>

#include "ssfn.h"

inline constexpr uSize PIXEL_BYTES = 4;


struct window::FontContext {
    SSFN::Font font;
};

window::FontContext* g_font_context = nullptr;

window::FontContext& default_context() {
    if (!g_font_context) {
        g_font_context = new window::FontContext();
    }
    return *g_font_context;
}

core::expected<window::OwningBitmap> window::OwningBitmap::create(u32 width, u32 height) {
    return create(width, height, width, height);
}
core::expected<window::OwningBitmap> window::OwningBitmap::create(u32 width, u32 height, u32 max_width,
                                                                  u32 max_height) {
    u32 stride = bek::align_up(width * 4u, 64u);
    uSize max_stride = bek::align_up(static_cast<uSize>(max_width) * 4ul, 64ul);
    uSize buf_size = bek::align_up(max_stride * static_cast<uSize>(max_height), 4096ul);

    auto allocate_res =
        EXPECTED_TRY(core::syscall::allocate(sc::INVALID_ADDRESS_VAL, buf_size, sc::AllocateFlags::None));
    return OwningBitmap{reinterpret_cast<u8*>(allocate_res), buf_size, stride, width, height};
}
core::expected<window::OwningBitmap> window::OwningBitmap::create_from_ipc(void* buffer, uSize buffer_size, u32 width,
                                                                           u32 height, u32 stride) {
    return OwningBitmap{static_cast<u8*>(buffer), buffer_size, stride, width, height};
}
window::OwningBitmap::OwningBitmap(OwningBitmap&& other) noexcept
    : m_buffer(bek::exchange(other.m_buffer, nullptr)),
      m_buffer_size(bek::exchange(other.m_buffer_size, 0)),
      m_stride(other.m_stride),
      m_width(other.m_width),
      m_height(other.m_height) {}
window::OwningBitmap& window::OwningBitmap::operator=(OwningBitmap&& other) noexcept {
    if (this == &other) return *this;
    m_buffer = bek::exchange(other.m_buffer, nullptr);
    m_buffer_size = bek::exchange(other.m_buffer_size, 0);
    m_stride = other.m_stride;
    m_width = other.m_width;
    m_height = other.m_height;
    return *this;
}
window::OwningBitmap::~OwningBitmap() {
    core::syscall::deallocate(reinterpret_cast<uPtr>(m_buffer), m_buffer_size);
}
window::OwningBitmap window::OwningBitmap::create_null() { return {nullptr, 0, 0, 0, 0}; }

window::RenderContext window::RenderContext::create(void* buffer, u32 stride, u32 width, u32 height) {
    return {
        .buffer = buffer,
        .byte_stride = stride,
        .pixel_width = width,
        .pixel_height = height,
        .confinement = Rect{{0, 0}, {static_cast<i32>(width), static_cast<i32>(height)}},
        .font_context = default_context(),
    };
}
window::Renderer::Renderer(const RenderContext& context, Rect reference_region): m_context(context), m_reference_region(reference_region) {}
void window::Renderer::paint_rect(Colour c, Rect location) {
    location.origin += m_reference_region.origin;
    VERIFY(location.is_within(m_reference_region));
    location = location.intersection(m_context.confinement);
    VERIFY(location.is_positive());
    for (int y = location.y(); y < location.y() + location.height(); y++) {
        auto* row_start = m_context.pixel_at(0, y);
        for (int x = location.x(); x < location.x() + location.width(); x++) {
            row_start[x] = c;
        }
    }
}
void window::Renderer::paint_border(Colour c, Rect location, u32 thickness_u) {
    int thickness = static_cast<int>(thickness_u);
    VERIFY(thickness >= location.height() && thickness >= location.width());
    location.origin += m_reference_region.origin;
    VERIFY(location.is_within(m_reference_region));
    location = location.intersection(m_context.confinement);
    VERIFY(location.is_positive());
    int y = location.y();
    for (; y < location.y() + thickness; y++) {
        auto* row_start = m_context.pixel_at(y, 0);
        for (int x = location.x(); x < location.x() + location.width(); x++) {
            row_start[x] = c;
        }
    }
    for (; y < location.y() + location.height() - thickness; y++) {
        auto* row_start = m_context.pixel_at(y, 0);
        for (int x = location.x(); x < location.x() + thickness; x++) {
            row_start[x] = c;
        }
        for (int x = location.x() + location.width() - thickness; x < location.x() + location.width(); x++) {
            row_start[x] = c;
        }
    }
    for (; y < location.y() + location.height(); y++) {
        auto* row_start = m_context.pixel_at(y, 0);
        for (int x = location.x(); x < location.x() + location.width(); x++) {
            row_start[x] = c;
        }
    }
}

window::Rect window::Renderer::paint_text(Colour c, bek::str_view text, Rect region, TextAlignment alignment) {
    int w, h, l, t;
    m_context.font_context.font.BBox(text, &w, &h, &l, &t);

    region.origin += m_reference_region.origin;
    region.y() += (region.height() - h) / 2;
    switch (alignment) {
        case TextAlignment::Left:
            break;
        case TextAlignment::Middle:
            region.x() += (region.width() - w) / 2;
            break;
        case TextAlignment::Right:
            region.x() += (region.width() - w);
            break;
    }
    region.height() = h;
    region.width() = w;

    ssfn_buf_t buf{
        .ptr = static_cast<u8*>(m_context.buffer),
        .w = static_cast<int>(m_context.pixel_width),
        .h = static_cast<int>(m_context.pixel_height),
        .p = static_cast<u16>(m_context.byte_stride),
        .x = region.x() + l,
        .y = region.y() + t,
        .fg = c,
        .bg = 0,
    };
    m_context.font_context.font.Render(&buf, text);
    return {region.origin - m_reference_region.origin, region.size};
}
void window::Renderer::paint_bitmap(const OwningBitmap& bitmap, Rect region, Vec bitmap_offset) {
    Rect bitmap_rect{bitmap_offset, region.size};
    VERIFY(bitmap_rect.is_within(Rect{{0, 0}, {static_cast<int>(bitmap.width()), static_cast<int>(bitmap.height())}}));
    region.origin += m_reference_region.origin;
    VERIFY(region.is_within(m_reference_region));
    VERIFY(region.is_positive());
    for (int row = 0; row < region.height(); row++) {
        bek::memcopy(m_context.pixel_at(region.x(), region.y() + row), bitmap.pixel_at(bitmap_rect.x(), bitmap_rect.y()+row), bitmap_rect.width()*PIXEL_BYTES);
    }
}