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

#ifndef BEKOS_LIBWINDOW_SSFN_H
#define BEKOS_LIBWINDOW_SSFN_H

#define SSFN_VERSION 0x0200

#include <bek/memory.h>
#include <bek/str.h>
#include <bek/types.h>

extern "C" {

/***** file format *****/

/* magic bytes */
#define SSFN_MAGIC "SFN2"
#define SSFN_COLLECTION "SFNC"
#define SSFN_ENDMAGIC "2NFS"

/* ligatures area */
#define SSFN_LIG_FIRST 0xF000
#define SSFN_LIG_LAST 0xF8FF

/* font family group in font type byte */
#define SSFN_TYPE_FAMILY(x) ((x) & 15)
#define SSFN_FAMILY_SERIF 0
#define SSFN_FAMILY_SANS 1
#define SSFN_FAMILY_DECOR 2
#define SSFN_FAMILY_MONOSPACE 3
#define SSFN_FAMILY_HAND 4

/* font style flags in font type byte */
#define SSFN_TYPE_STYLE(x) (((x) >> 4) & 15)
#define SSFN_STYLE_REGULAR 0
#define SSFN_STYLE_BOLD 1
#define SSFN_STYLE_ITALIC 2
#define SSFN_STYLE_USRDEF1 4 /* user defined variant 1 */
#define SSFN_STYLE_USRDEF2 8 /* user defined variant 2 */

/* contour commands */
#define SSFN_CONTOUR_MOVE 0
#define SSFN_CONTOUR_LINE 1
#define SSFN_CONTOUR_QUAD 2
#define SSFN_CONTOUR_CUBIC 3

/* glyph fragments, kerning groups and hinting grid info */
#define SSFN_FRAG_CONTOUR 0
#define SSFN_FRAG_BITMAP 1
#define SSFN_FRAG_PIXMAP 2
#define SSFN_FRAG_KERNING 3
#define SSFN_FRAG_HINTING 4

/* main SSFN header, 32 bytes */

#define _pack __attribute__((packed))

typedef struct {
    u8 magic[4];         /* SSFN magic bytes */
    u32 size;            /* total size in bytes */
    u8 type;             /* font family and style */
    u8 features;         /* format features and revision */
    u8 width;            /* overall width of the font */
    u8 height;           /* overall height of the font */
    u8 baseline;         /* horizontal baseline in grid pixels */
    u8 underline;        /* position of under line in grid pixels */
    u16 fragments_offs;  /* offset of fragments table */
    u32 characters_offs; /* characters table offset */
    u32 ligature_offs;   /* ligatures table offset */
    u32 kerning_offs;    /* kerning table offset */
    u32 cmap_offs;       /* color map offset */
} _pack ssfn_font_t;

/***** renderer API *****/
#define SSFN_FAMILY_ANY 0xff    /* select the first loaded font */
#define SSFN_FAMILY_BYNAME 0xfe /* select font by its unique name */

/* additional styles not stored in fonts */
#define SSFN_STYLE_UNDERLINE 16   /* under line glyph */
#define SSFN_STYLE_STHROUGH 32    /* strike through glyph */
#define SSFN_STYLE_NOAA 64        /* no anti-aliasing */
#define SSFN_STYLE_NOKERN 128     /* no kerning */
#define SSFN_STYLE_NODEFGLYPH 256 /* don't draw default glyph */
#define SSFN_STYLE_NOCACHE 512    /* don't cache rasterized glyph */
#define SSFN_STYLE_NOHINTING 1024 /* no auto hinting grid (not used as of now) */
#define SSFN_STYLE_RTL 2048       /* render right-to-left */
#define SSFN_STYLE_ABS_SIZE 4096  /* scale absoulte height */
#define SSFN_STYLE_NOSMOOTH 8192  /* no edge-smoothing for bitmaps */

/* error codes */
#define SSFN_OK 0            /* success */
#define SSFN_ERR_ALLOC (-1)    /* allocation error */
#define SSFN_ERR_BADFILE (-2)  /* bad SSFN file format */
#define SSFN_ERR_NOFACE (-3)   /* no font face selected */
#define SSFN_ERR_INVINP (-4)   /* invalid input */
#define SSFN_ERR_BADSTYLE (-5) /* bad style */
#define SSFN_ERR_BADSIZE (-6)  /* bad size */
#define SSFN_ERR_NOGLYPH (-7)  /* glyph (or kerning info) not found */

#define SSFN_SIZE_MAX 192 /* biggest size we can render */
#define SSFN_ITALIC_DIV 4 /* italic angle divisor, glyph top side pushed width / this pixels */
#define SSFN_PREC 4       /* precision in bits */

/* destination frame buffer context */
typedef struct {
    u8* ptr; /* pointer to the buffer */
    int w;   /* width (positive: ARGB, negative: ABGR pixels) */
    int h;   /* height */
    u16 p;   /* pitch, bytes per line */
    int x;   /* cursor x */
    int y;   /* cursor y */
    u32 fg;  /* foreground color */
    u32 bg;  /* background color */
} ssfn_buf_t;

/* cached bitmap struct */
#define SSFN_DATA_MAX 65536
typedef struct {
    u16 p;                  /* data buffer pitch, bytes per line */
    u8 h;                   /* data buffer height */
    u8 o;                   /* overlap of glyph, scaled to size */
    u8 x;                   /* advance x, scaled to size */
    u8 y;                   /* advance y, scaled to size */
    u8 a;                   /* ascender, scaled to size */
    u8 d;                   /* descender, scaled to size */
    u8 data[SSFN_DATA_MAX]; /* data buffer */
} ssfn_glyph_t;

/* character metrics */
typedef struct {
    u8 t; /* type and overlap */
    u8 n; /* number of fragments */
    u8 w; /* width */
    u8 h; /* height */
    u8 x; /* advance x */
    u8 y; /* advance y */
} ssfn_chr_t;

#ifdef SSFN_PROFILING
#include <string.h>
#include <sys/time.h>
#endif

/* renderer context */
typedef struct {
#ifdef SSFN_MAXLINES
    const ssfn_font_t* fnt[5][16]; /* static font registry */
#else
    const ssfn_font_t** fnt[5]; /* dynamic font registry */
#endif
    const ssfn_font_t* s; /* explicitly selected font */
    const ssfn_font_t* f; /* font selected by best match */
    ssfn_glyph_t ga;      /* glyph sketch area */
    ssfn_glyph_t* g;      /* current glyph pointer */
#ifdef SSFN_MAXLINES
    u16 p[SSFN_MAXLINES * 2];
#else
    ssfn_glyph_t*** c[17]; /* glyph cache */
    u16* p;
    char** bufs; /* allocated extra buffers */
#endif
    ssfn_chr_t* rc; /* pointer to current character */
    int numbuf, lenbuf, np, ap, ox, oy, ax;
    int mx, my, lx, ly; /* move to coordinates, last coordinates */
    int len[5];         /* number of fonts in registry */
    int family;         /* required family */
    int style;          /* required style */
    int size;           /* required size */
    int line;           /* calculate line height */
#ifdef SSFN_PROFILING
    u64 lookup, raster, blit, kern; /* profiling accumulators */
#endif
} ssfn_t;

/***** API function protoypes *****/

u32 ssfn_utf8(char** str); /* decode UTF-8 sequence */

/* normal renderer */
int ssfn_load(ssfn_t* ctx, const void* data);                                     /* add an SSFN to context */
int ssfn_select(ssfn_t* ctx, int family, const char* name, int style, int size);  /* select font to use */
int ssfn_render(ssfn_t* ctx, ssfn_buf_t* dst, const char* str);                   /* render a glyph to a pixel buffer */
int ssfn_bbox(ssfn_t* ctx, const char* str, int* w, int* h, int* left, int* top); /* get bounding box */
ssfn_buf_t* ssfn_text(ssfn_t* ctx, const char* str, unsigned int fg); /* renders text to a newly allocated buffer */
int ssfn_mem(ssfn_t* ctx);                                            /* return how much memory is used */
void ssfn_free(ssfn_t* ctx);                                          /* free context */
#define ssfn_error(err) (err < 0 && err >= -7 ? ssfn_errstr[-err] : "Unknown error") /* return string for error code \
                                                                                      */
extern const char* ssfn_errstr[];
}

/*** SSFN C++ Wrapper Class ***/
namespace SSFN {
class Font {
    ssfn_t ctx;

public:
    Font(): ctx{} { bek::memset(&this->ctx, 0, sizeof(ssfn_t)); }
    ~Font() { ssfn_free(&this->ctx); }

    int Load(const void* data) { return ssfn_load(&this->ctx, data); }
    int Select(int family, bek::str_view name, int style, int size) {
        return ssfn_select(&this->ctx, family, (char*)name.data(), style, size);
    }
    int Select(int family, const char* name, int style, int size) {
        return ssfn_select(&this->ctx, family, name, style, size);
    }
    int Render(ssfn_buf_t* dst, bek::str_view str) { return ssfn_render(&this->ctx, dst, (const char*)str.data()); }
    int Render(ssfn_buf_t* dst, const char* str) { return ssfn_render(&this->ctx, dst, str); }
    int BBox(bek::str_view str, int* w, int* h, int* left, int* top) {
        return ssfn_bbox(&this->ctx, (const char*)str.data(), w, h, left, top);
    }
    int BBox(const char* str, int* w, int* h, int* left, int* top) {
        return ssfn_bbox(&this->ctx, str, w, h, left, top);
    }
    ssfn_buf_t* Text(bek::str_view str, unsigned int fg) { return ssfn_text(&this->ctx, (const char*)str.data(), fg); }
    ssfn_buf_t* Text(const char* str, unsigned int fg) { return ssfn_text(&this->ctx, str, fg); }
    int LineHeight() const { return this->ctx.line ? this->ctx.line : this->ctx.size; }
    int Mem() { return ssfn_mem(&this->ctx); }
    static bek::str_view ErrorStr(int err) { return bek::str_view(ssfn_error(err)); }
};
}  // namespace SSFN

#endif  // BEKOS_LIBWINDOW_SSFN_H
