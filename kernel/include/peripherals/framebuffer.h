/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef BEKOS_FRAMEBUFFER_H
#define BEKOS_FRAMEBUFFER_H

#include "library/types.h"

struct colour {
    colour(u8 r, u8 g, u8 b, u8 a) {
        red = r;
        green = g;
        blue = b;
        alpha = a;
    }

    explicit  colour(u32 c) {
        raw = c;
    }
    union {
        u32 raw;
        struct {
            u8 red;
            u8 green;
            u8 blue;
            u8 alpha;
        };
    };
};

struct framebuffer_info {
    u32 width;
    u32 height;
    u32 depth;
    u32 pitch;
    void* buffer;
    u32 buffer_size;
};

bool allocate_framebuffer(framebuffer_info& info);

class framebuffer {
public:
    explicit framebuffer(framebuffer_info t_info);
    void clear(colour c);
    void setPixel(u32 row, u32 col, colour c);

    u32 getHeight() const;
    u32 getWidth() const;
private:
    framebuffer_info m_info;
};

class char_blitter {
public:
    explicit char_blitter(framebuffer t_fb, colour fg, colour bg);

    void putChar(char c);
    void seek(u32 row, u32 col);

private:
    void blitChar(char c);

    framebuffer m_framebuffer;
    u32 n_cols;
    u32 n_rows;

    u32 m_row;
    u32 m_col;

    colour m_fg;
    colour m_bg;
};

#endif  // BEKOS_FRAMEBUFFER_H
