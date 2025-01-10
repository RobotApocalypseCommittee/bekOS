/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024-2025 Bekos Contributors
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

#include "api/protocols/fb.h"
#include "api/protocols/kb.h"
#include "bek/format.h"
#include "bek/own_ptr.h"
#include "core/device.h"
#include "core/io.h"
#include "core/syscall.h"

template <typename... Args>
void dbgln(bek::str_view view, Args&&... args) {
    core::fprintln(core::stdout, view, args...);
}

struct Colour {
    u8 r;
    u8 g;
    u8 b;
};

inline constexpr uSize KEYCODE_COUNT = 57;
char keycode_mapping[KEYCODE_COUNT][2] = {
    {},         {},         {},         {},         {'a', 'A'},   {'b', 'B'}, {'c', 'C'},   {'d', 'D'},   {'e', 'E'},
    {'f', 'F'}, {'g', 'G'}, {'h', 'H'}, {'i', 'I'}, {'j', 'J'},   {'k', 'K'}, {'l', 'L'},   {'m', 'M'},   {'n', 'N'},
    {'o', 'O'}, {'p', 'P'}, {'q', 'Q'}, {'r', 'R'}, {'s', 'S'},   {'t', 'T'}, {'u', 'U'},   {'v', 'V'},   {'w', 'W'},
    {'x', 'X'}, {'y', 'Y'}, {'z', 'Z'}, {'1', '!'}, {'2', '@'},   {'3', '#'}, {'4', '$'},   {'5', '%'},   {'6', '^'},
    {'7', '&'}, {'8', '*'}, {'9', '('}, {'0', ')'}, {'\n', '\n'}, {0, 0},     {'\b', '\b'}, {'\t', '\t'}, {' ', ' '},
    {'-', '_'}, {'=', '+'}, {'[', '{'}, {']', '}'}, {'\\', '|'},  {0, 0},     {';', ':'},   {'\'', '"'},  {0, 0},
    {',', '<'}, {'.', '>'}, {'/', '?'},
};

constexpr inline uSize BITFONT_HEADER_SIZE = 32;

const unsigned char unisig8[BITFONT_HEADER_SIZE + 1] = "\xffUnisig\x00\x0a\x0d\x0a\x12io.lassi.dumbfont8\x00\x00";

const unsigned char unisig16[BITFONT_HEADER_SIZE + 1] = "\xffUnisig\x00\x0a\x0d\x0a\x13io.lassi.dumbfont16\x00";

const unsigned char unisig32[BITFONT_HEADER_SIZE + 1] = "\xffUnisig\x00\x0a\x0d\x0a\x13io.lassi.dumbfont32\x00";

const unsigned char unisig64[BITFONT_HEADER_SIZE + 1] = "\xffUnisig\x00\x0a\x0d\x0a\x13io.lassi.dumbfont64\x00";

struct Font {
    struct DF16Glyph {
        u16 rows[16];
    };
    static core::expected<Font> load(bek::str_view path) {
        sc::Stat stat;
        auto ed = EXPECTED_TRY(core::syscall::open(path, sc::OpenFlags::Read, sc::INVALID_ENTITY_ID, &stat));
        core::fprintln(core::stdout, "Opened font file {}, size {}"_sv, path, stat.size);
        VERIFY(stat.size > BITFONT_HEADER_SIZE);
        bek::vector<u8> header(BITFONT_HEADER_SIZE);
        EXPECTED_TRY(core::syscall::read(ed, 0, header.data(), BITFONT_HEADER_SIZE));
        if (bek::mem_compare(header.data(), unisig16, BITFONT_HEADER_SIZE)) {
            core::fprintln(core::stdout, "Font file is not a dumbfont16 format."_sv);
            return EINVAL;
        }
        auto remaining_size = stat.size - BITFONT_HEADER_SIZE;
        auto num_chars = remaining_size / sizeof(DF16Glyph);
        bek::vector<DF16Glyph> glyphs(num_chars);
        EXPECTED_TRY(core::syscall::read(ed, BITFONT_HEADER_SIZE, glyphs.data(), num_chars * sizeof(DF16Glyph)));
        return Font{ed, bek::move(glyphs)};
    }

    void blit_char(char c, u8* start, uSize byte_stride, u32 colour_fg, u32 colour_bg) {
        VERIFY((unsigned)c < glyphs.size());
        auto& glyph = glyphs[c];
        for (auto row : glyph.rows) {
            for (uSize bit = 0; row != 0 && bit < 16; row >>= 1, bit++) {
                *reinterpret_cast<u32*>(start + bit * 4) = (row & 1u) ? colour_fg : colour_bg;
            }
            start += byte_stride;
        }
    }
    long ed;
    bek::vector<DF16Glyph> glyphs;
};

struct FramebufferDevice {
    static core::expected<bek::own_ptr<FramebufferDevice>> create(bek::str_view path) {
        auto ed = EXPECTED_TRY(core::syscall::open_device(path));
        dbgln("Opened framebuffer device: {}"_sv, ed);

        auto display_info = protocol::fb::DisplayInfoMessage{protocol::fb::GetDisplayInfo, {}};
        if (EXPECTED_TRY(core::syscall::message(ed, 0, &display_info, sizeof(display_info))) != sizeof(display_info)) {
        }

        dbgln("Got display info"_sv);

        auto fb_info = protocol::fb::MapMessage{protocol::fb::MapFramebuffer, 0, 0, 0, 0, 0};

        if (EXPECTED_TRY(core::syscall::message(ed, 0, &fb_info, sizeof(fb_info))) != sizeof(fb_info)) {
        }

        core::fprintln(core::stdout, "Opened display of {}x{}."_sv, display_info.info.width, display_info.info.height);
        core::fprintln(core::stdout, "  with framebuffer of {}x{} at {:Xl}"_sv, fb_info.pixel_width,
                       fb_info.pixel_height, fb_info.buffer);

        return bek::make_own<FramebufferDevice>(ed, display_info.info, fb_info);
    }

private:
    long m_ed;
    protocol::fb::DisplayInfo m_info;
    protocol::fb::MapMessage m_framebuffer;

public:
    FramebufferDevice(long ed, const protocol::fb::DisplayInfo& info, protocol::fb::MapMessage framebuffer)
        : m_ed(ed), m_info(info), m_framebuffer(framebuffer) {}

    u16 width() const { return m_info.width; }
    u16 height() const { return m_info.height; }
    u64 stride() const { return m_framebuffer.row_stride; }
    u8* framebuffer() const { return reinterpret_cast<u8*>(m_framebuffer.buffer); }

    void set_pixel(u16 x, u16 y, Colour colour);

    void flush() {
        auto flush_msg = protocol::fb::FlushRectMessage{protocol::fb::FlushRect, {0, 0, height(), width()}};
        auto res = core::syscall::message(m_ed, 0, &flush_msg, sizeof(flush_msg));
        if (res.has_error()) {
            core::fprintln(core::stdout, "Failed to flush!"_sv);
        } else {
            core::fprintln(core::stdout, "Flushed!"_sv);
        }
    }
};

struct KbdDevice {
    static core::expected<bek::own_ptr<KbdDevice>> create(bek::str_view path) {
        auto ed = EXPECTED_TRY(core::syscall::open_device(path));
        dbgln("Opened keyboard device: {}"_sv, ed);
        protocols::kb::GetReportMessage message{protocols::kb::GetReport, {}};
        EXPECTED_TRY(core::syscall::message(ed, 0, &message, sizeof(message)));
        return bek::own_ptr{new KbdDevice(message.report, ed)};
    }

    char get_update() {
        protocols::kb::GetReportMessage message{protocols::kb::GetReport, {}};
        EXPECTED_TRY(core::syscall::message(m_ed, 0, &message, sizeof(message)));

        auto old_report = m_last_report;
        m_last_report = message.report;
        for (u8 new_c_byte : message.report.keys) {
            if (new_c_byte && new_c_byte < KEYCODE_COUNT) {
                bool is_new = true;
                for (u8 old_c_byte : old_report.keys) {
                    if (old_c_byte == new_c_byte) {
                        // Not new
                        is_new = false;
                    }
                }
                if (is_new) {
                    bool is_shift = message.report.modifier_keys & (0x2 | 0x20);
                    return keycode_mapping[new_c_byte][is_shift];
                }
            }
        }
        return 0;
    }

private:
    KbdDevice(const protocols::kb::Report& last_report, long ed) : m_last_report(last_report), m_ed(ed) {}

    protocols::kb::Report m_last_report;
    long m_ed;
};

void FramebufferDevice::set_pixel(u16 x, u16 y, Colour colour) {
    u32 colour_int = (u32)colour.r | ((u32)colour.g << 8) | ((u32)colour.b << 16);
    uPtr ptr = m_framebuffer.buffer + ((uPtr)y * (uPtr)m_framebuffer.row_stride) + 4 * (uPtr)x;
    *reinterpret_cast<u32*>(ptr) = colour_int;
}

Colour HsvToRgb(u8 h, u8 s, u8 v) {
    Colour rgb;
    unsigned char region, remainder, p, q, t;

    if (s == 0) {
        rgb.r = v;
        rgb.g = v;
        rgb.b = v;
        return rgb;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.r = v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = v;
            rgb.b = t;
            break;
        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = v;
            break;
        default:
            rgb.r = v;
            rgb.g = p;
            rgb.b = q;
            break;
    }

    return rgb;
}

int main(int argc, char** argv) {
    core::expected<int> pid = core::syscall::get_pid();
    if (pid.has_error()) {
        return pid.error();
    }

    core::fprintln(core::stdout, "Hello from process {}!"_sv, pid.value());
    auto v = core::Device::get_devices();
    core::fprintln(core::stdout, "Found {} devices:"_sv, v.size());
    for (auto& dev : v) {
        core::fprintln(core::stdout, "{}  {}"_sv, (uSize)dev.protocol, dev.name.view());
    }

    auto graphics_devices = core::Device::get_devices(DeviceProtocol::FramebufferProvider);
    if (graphics_devices.size() != 1) {
        core::fprintln(core::stdout, "Found {} graphics devices. Exiting."_sv, graphics_devices.size());
        return 0;
    }

    auto fb_r = FramebufferDevice::create(graphics_devices[0].name.view());
    if (fb_r.has_error()) {
        core::fprintln(core::stdout, "Failed to initialise graphics device {}"_sv, (u32)fb_r.error());
        return 0;
    }

    auto& fb = *fb_r.value();

    auto kbd_devices = core::Device::get_devices(DeviceProtocol::Keyboard);
    if (kbd_devices.size() != 1) {
        core::fprintln(core::stdout, "Found {} keyboard devices. Exiting."_sv, kbd_devices.size());
        return 0;
    }

    auto kb_r = KbdDevice::create(kbd_devices[0].name.view());
    if (kb_r.has_error()) {
        core::fprintln(core::stdout, "Failed to initialise keyboard device {}"_sv, (u32)kb_r.error());
        return 0;
    }

    auto& kbd = *kb_r.value();

    auto font_r = Font::load("/res/font.dumbfont"_sv);
    if (font_r.has_error()) {
        core::fprintln(core::stdout, "Failed to load font: {}"_sv, (u32)font_r.error());
        return 0;
    }
    auto& font = font_r.value();

    core::fprintln(core::stdout, "{}"_sv, EAGAIN);

    bek::str_view string = "Hello, world!\n"_sv;

    u8* start = fb.framebuffer();
    auto stride = fb.stride();
    uSize r = 0;
    uSize c = 0;
    auto end_column = fb.width() / 16u;

    u32 bg = 0x000000FF;
    u32 fg = 0xFFFFFFFF;

    for (char ch : string) {
        if (ch == '\n' || c >= end_column) {
            c = 0;
            r++;
            start = fb.framebuffer() + stride * r * 16;
            if (ch == '\n') continue;
        }
        font.blit_char(ch, start, stride, fg, bg);
        start += 4 * 16;
        c += 1;
    }
    fb.flush();

    auto pipe_result = core::syscall::create_pipe({0, 0, false, true});
    if (pipe_result.has_error()) {
        dbgln("create_pipe for stdout failed: {}"_sv, pipe_result.error());
        return 0;
    }
    auto stdout_handles = pipe_result.value();

    pipe_result = core::syscall::create_pipe({0, 0, true, true});
    if (pipe_result.has_error()) {
        dbgln("create_pipe for stdin failed: {}"_sv, pipe_result.error());
        return 0;
    }
    auto stdin_handles = pipe_result.value();

    core::stdout.flush();
    core::stdin.flush();
    auto fork_result = core::syscall::fork();
    if (fork_result.has_error()) {
        dbgln("Fork failed: {}"_sv, fork_result.error());
    } else if (fork_result.value() == 0) {
        // We now equip our new stdin and stdout.
        core::syscall::close(stdout_handles.read_handle);
        core::syscall::close(stdin_handles.write_handle);

        if (auto dup_r = core::syscall::duplicate(stdin_handles.read_handle, 1, 0); dup_r.has_error()) {
            dbgln("Dup failed: {}"_sv, dup_r.error());
        }
        if (auto dup_r = core::syscall::duplicate(stdout_handles.write_handle, 0, 0); dup_r.has_error()) {
            dbgln("Dup failed: {}"_sv, dup_r.error());
        }
        if (auto dup_r = core::syscall::duplicate(stdout_handles.write_handle, 2, 0); dup_r.has_error()) {
            dbgln("Dup failed: {}"_sv, dup_r.error());
        }

        auto exec_result = core::syscall::exec("/bin/shell"_sv, {}, {});
        if (exec_result.has_error()) {
            dbgln("Exec failed: {}"_sv, fork_result.error());
        } else {
            dbgln("Very very bad."_sv);
        }
        while (true) {
            core::syscall::sleep(1'000'000);
            dbgln("Ping!"_sv);
        }
    } else {
        dbgln("Fork - parent process. Child process: {}."_sv, fork_result.value());
    }

    while (true) {
        auto ch = kbd.get_update();
        // If true, we send to child stdin.
        if (ch) {
            if (auto write_r = core::syscall::write(stdin_handles.write_handle, sc::INVALID_OFFSET_VAL, &ch, 1);
                write_r.has_error()) {
                dbgln("Write to stdin pipe failed: {}"_sv, write_r.error());
            }
        }
        // Next, we see if there's a chars we need to read!
        char temp_buffer[100];
        auto read_pipe_res =
            core::syscall::read(stdout_handles.read_handle, sc::INVALID_OFFSET_VAL, &temp_buffer, sizeof(temp_buffer));
        if (read_pipe_res.has_error() && read_pipe_res.error() != EAGAIN) {
            dbgln("Failed to read from pipe: {}"_sv, read_pipe_res.error());
        } else if (read_pipe_res.has_value() && read_pipe_res.value() > 0) {
            uSize chars_read = read_pipe_res.value();
            dbgln("Read {} chars from pipe: {}"_sv, chars_read,
                  bek::str_view{temp_buffer, chars_read});
            for (uSize i = 0; i < chars_read; i++) {
                char ch = temp_buffer[i];
                if (ch == '\n' || c >= end_column) {
                    c = 0;
                    r++;
                    start = fb.framebuffer() + stride * r * 16;
                    if (ch == '\n') continue;
                } else if (ch == '\b') {
                    if (c > 0) {
                        c--;
                        start -= 16 * 4;
                        font.blit_char(' ', start, stride, fg, bg);
                        continue;
                    }
                }
                font.blit_char(ch, start, stride, fg, bg);
                start += 4 * 16;
                c += 1;
            }
            fb.flush();
        }
        asm volatile("nop");
    }

    return 0;
}