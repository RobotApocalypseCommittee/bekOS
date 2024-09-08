/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
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

#include "peripherals/raspberry/framebuffer.h"

#include "bek/assertions.h"
#include "library/debug.h"
#include "peripherals/property_tags.h"

constexpr inline u8 font[] = {
#include "font.h"
};

using DBG = DebugScope<"FB(RPi)", true>;

struct framebuffer_allocation_msg {
    PropertyTagSetFBSize setPhys;
    PropertyTagSetFBSize setVirt;
    PropertyTagSetFBDepth setDepth;
    PropertyTagAllocateFB allocateFB;
    PropertyTagGetFBPitch getPitch;
} __attribute__((packed));

bool allocate_framebuffer(framebuffer_info& info) {
    DBG::dbgln("Allocating framebuffer"_sv);
    framebuffer_allocation_msg fb_msg{.setPhys = PropertyTagSetFBSize(true, false, info.width, info.height),
                                      .setVirt = PropertyTagSetFBSize(true, true, info.width, info.height),
                                      .setDepth = PropertyTagSetFBDepth(true, info.depth),
                                      .allocateFB = PropertyTagAllocateFB(16),
                                      .getPitch = PropertyTagGetFBPitch()};
    property_tags propTags;

    DBG::dbgln("Allocation message:"_sv);
    // ASSERT((&fb_msg, sizeof(framebuffer_allocation_msg));

    DBG::dbgln("Requesting tag."_sv);
    bool res = propTags.request_tags(&fb_msg, sizeof(framebuffer_allocation_msg));
    if (!res) {
        DBG::dbgln("Framebuffer allocation failed."_sv);
        return false;
    }
    DBG::dbgln("Allocated framebuffer:"_sv);
    info.width = fb_msg.setPhys.width;
    info.height = fb_msg.setPhys.height;
    info.depth = fb_msg.setDepth.depth;
    info.pitch = fb_msg.getPitch.pitch;
    info.buffer = (void*)mapped_address(fb_msg.allocateFB.base_address & 0x3FFFFFFF);
    info.buffer_size = fb_msg.allocateFB.allocation_size;
    DBG::dbgln("Allocated framebuffer: width={}, height={}, depth={}, pitch={}, addr={:XL}, size={}"_sv, info.width,
               info.height, info.depth, info.pitch, reinterpret_cast<uPtr>(info.buffer), info.buffer_size);
    return true;
}

framebuffer::framebuffer(framebuffer_info t_info) : m_info(t_info) {}
void framebuffer::clear(colour c) {
    for (unsigned int row = 0; row < m_info.height; row++) {
        for (unsigned int col = 0; col < m_info.width; col++) {
            setPixel(row, col, c);
        }
    }
}
void framebuffer::setPixel(u32 row, u32 col, colour c) {
    u8* pixel_loc = (u8*)m_info.buffer + row * m_info.pitch + col * m_info.depth / 8;
    bek::memcopy(pixel_loc, &c, m_info.depth / 8);
}
u32 framebuffer::getHeight() const { return m_info.height; }
u32 framebuffer::getWidth() const { return m_info.width; }

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

char_blitter::char_blitter(framebuffer t_fb, colour fg, colour bg)
    : m_framebuffer(t_fb), m_row(0), m_col(0), m_fg(fg), m_bg(bg) {
    n_cols = m_framebuffer.getWidth() / CHAR_WIDTH;
    n_rows = m_framebuffer.getHeight() / CHAR_HEIGHT;
}
void char_blitter::blitChar(char c) {
    VERIFY(c >= 32 && c <= 126);
    c -= 32;
    for (int roffset = 0; roffset < CHAR_HEIGHT; roffset++) {
        for (int coffset = 0; coffset < CHAR_WIDTH; coffset++) {
            m_framebuffer.setPixel(m_row * CHAR_HEIGHT + roffset, m_col * CHAR_WIDTH + coffset,
                                   (font[c * CHAR_HEIGHT + roffset] & (0b10000000 >> coffset)) ? m_fg : m_bg);
        }
    }
}
void char_blitter::putChar(char c) {
    if (c >= 32 && c <= 126) {
        blitChar(c);
        m_col++;
    } else if (c == '\n') {
        m_row++;
        m_col = 0;
    }

    if (m_col == n_cols) {
        m_row++;
        m_col = 0;
    }
    if (m_row == n_rows) {
        m_row = 0;
        m_framebuffer.clear(m_bg);
    }
}
void char_blitter::seek(u32 row, u32 col) {
    ASSERT(row < n_rows && col < n_cols);
    m_row = row;
    m_col = col;
}
