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

#ifndef BEKOS_WINDOW_GFX_H
#define BEKOS_WINDOW_GFX_H

#include <bek/own_ptr.h>
#include <bek/str.h>
#include <core/error.h>

#include "core.h"

namespace window {

struct FontContext;

bek::own_ptr<FontContext> create_blank_context();

class OwningBitmap {
public:
    static core::expected<OwningBitmap> create(u32 width, u32 height);
    static core::expected<OwningBitmap> create(u32 width, u32 height, u32 max_width, u32 max_height);
    static core::expected<OwningBitmap> create_from_ipc(void* buffer, uSize buffer_size, u32 width, u32 height,
                                                        u32 stride);

    u8* buffer() const { return m_buffer; }
    u32 stride() const { return m_stride; }
    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    uSize buffer_size() const { return m_buffer_size; }
    u32* pixel_at(u32 x, u32 y) const {
        return reinterpret_cast<u32*>(reinterpret_cast<u8*>(m_buffer) +
                                      static_cast<u64>(m_stride) * static_cast<u64>(y)) +
               x;
    }

    /**
     * resizes the buffer.
     * @return True indicates the buffer had to be reallocated, false otherwise.
     */
    [[nodiscard]] bool resize(u32 width, u32 height);

    OwningBitmap(const OwningBitmap& other) = delete;
    OwningBitmap& operator=(const OwningBitmap& other) = delete;
    OwningBitmap(OwningBitmap&& other) noexcept;
    OwningBitmap& operator=(OwningBitmap&& other) noexcept;
    ~OwningBitmap();

private:
    static OwningBitmap create_null();
    OwningBitmap(u8* buffer, uSize buffer_size, u32 stride, u32 width, u32 height)
        : m_buffer(buffer), m_buffer_size(buffer_size), m_stride(stride), m_width(width), m_height(height) {}
    u8* m_buffer;
    uSize m_buffer_size;
    u32 m_stride;
    u32 m_width;
    u32 m_height;
};

struct RenderContext {
    /// Buffer, assumed to be in ARGB 32-bit format.
    void* buffer;
    u32 byte_stride;
    u32 pixel_width;
    u32 pixel_height;
    /// Painting need only be confined to this region.
    Rect confinement;

    FontContext& font_context;

    ALWAYS_INLINE u32* pixel_at(u32 x, u32 y) const {
        return reinterpret_cast<u32*>(static_cast<u8*>(buffer) + static_cast<u64>(byte_stride) * static_cast<u64>(y)) +
               x;
    }

    static RenderContext create(void* buffer, u32 stride, u32 width, u32 height);

    Rect render_rect() const { return {{0, 0}, {static_cast<int>(pixel_width), static_cast<int>(pixel_height)}}; }
};

enum class TextAlignment { Left, Middle, Right };

class Renderer {
public:
    /// A Renderer draws to a buffer provided by context. It treats the provided reference region as the origin and
    /// extent of rendering.
    /// @param context
    /// @param reference_region
    Renderer(const RenderContext& context, Rect reference_region);

    void paint_rect(Colour c, Rect location);
    void paint_border(Colour c, Rect location, u32 thickness);
    Rect paint_text(Colour c, bek::str_view text, Rect region, TextAlignment alignment);
    void paint_bitmap(const OwningBitmap& bitmap, Rect region, Vec bitmap_offset);

private:
    const RenderContext& m_context;
    Rect m_reference_region;
};
}  // namespace window

#endif  // BEKOS_WINDOW_GFX_H
