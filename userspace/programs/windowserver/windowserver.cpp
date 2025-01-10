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

#include <api/protocols/fb.h>
#include <api/protocols/kb.h>
#include <api/protocols/mouse.h>
#include <bek/optional.h>
#include <bek/own_ptr.h>
#include <core/device.h>
#include <core/io.h>
#include <core/syscall.h>
#include <window/Window.gen.h>


u64 current_time;
i32 starting_coords;

template <typename... Args>
void dbgln(bek::str_view view, Args&&... args) {
    core::fprintln(core::stdout, view, args...);
}

struct FramebufferDevice {
    static core::expected<bek::own_ptr<FramebufferDevice>> create(bek::str_view path) {
        auto ed = EXPECTED_TRY(core::syscall::open_device(path));
        dbgln("Opened framebuffer device: {}"_sv, ed);

        auto display_info = protocol::fb::DisplayInfoMessage{protocol::fb::GetDisplayInfo, {}};
        if (EXPECTED_TRY(core::syscall::message(ed, 0, &display_info, sizeof(display_info))) != sizeof(display_info)) {
        }

        dbgln("Got display info"_sv);

        auto fb_info = protocol::fb::MapMessage{protocol::fb::MapFramebuffer, 0, 0, 0, 0, 0};

        if (EXPECTED_TRY(core::syscall::message(ed, 0, &fb_info, sizeof(fb_info))) != 0) {
            dbgln("Bad framebuffer response"_sv);
            return EFAIL;
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

    void flush() {
        auto flush_msg = protocol::fb::FlushRectMessage{protocol::fb::FlushRect, {0, 0, height(), width()}};
        auto res = core::syscall::message(m_ed, 0, &flush_msg, sizeof(flush_msg));
        if (res.has_error()) {
            core::fprintln(core::stdout, "Failed to flush!"_sv);
        }
    }
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

struct MouseDevice {
    static core::expected<bek::own_ptr<MouseDevice>> create(bek::str_view path, window::Rect bounds) {
        auto ed = EXPECTED_TRY(core::syscall::open_device(path));
        dbgln("Opened mouse device: {}"_sv, ed);
        return bek::own_ptr{new MouseDevice(bounds, ed)};
    }

    ErrorCode update() {
        protocols::mouse::GetReportMessage message{protocols::mouse::GetReport, {}};
        EXPECTED_TRY(core::syscall::message(m_ed, 0, &message, sizeof(message)));

        if (message.report.sequence_number != m_last_report.sequence_number) {
            // dbgln("New Mouse Message: ({}, {})"_sv, message.report.delta_x, message.report.delta_y);
            if (message.report.sequence_number - m_last_report.sequence_number > 1) {
                dbgln("Caution: missed sequence number: {} -> {}"_sv, m_last_report.sequence_number,
                      message.report.sequence_number);
            }
            m_last_report = message.report;
            m_location.x = bek::max(m_bounds.x(), bek::min(m_bounds.right(), m_location.x + message.report.delta_x));
            m_location.y = bek::max(m_bounds.y(), bek::min(m_bounds.bottom(), m_location.y + message.report.delta_y));
        }
        return ESUCCESS;
    }

    window::Vec position() const { return m_location; }

    bool is_clicked(u8 button) const { return m_last_report.buttons & (1 << button); }

private:
    MouseDevice(window::Rect bounds, long ed) : m_bounds(bounds), m_ed(ed) {}
    protocols::mouse::Report m_last_report{};
    window::Vec m_location{};
    window::Rect m_bounds;
    long m_ed;
};

class WindowServerConnection : public window::WindowServerRaw {
public:
    struct Window {
        u32 id;
        bek::optional<u32> current_surface_id;
        window::Rect placement;
    };
    explicit WindowServerConnection(int fd) : WindowServerRaw(fd) {}
    ~WindowServerConnection() override = default;




    void on_create_surface(u32 id, window::OwningBitmap region) override {
        // TODO: Replacing surfaces.
        for (auto& surf : m_surfaces) {
            if (surf.first == id) return;
        }
        m_surfaces.push_back(bek::pair{id, bek::move(region)});
        dbgln("Created surface {} ({}x{})"_sv, id, m_surfaces.back().second.width(), m_surfaces.back().second.height());
    }
    void on_create_window(u32 id, window::Vec requested_size) override {
        for (auto& win : m_windows) {
            if (win.id == id) return;
        }
        dbgln("Created window {}"_sv, id);
        m_windows.push_back(Window{
            .id = id,
            .current_surface_id = bek::nullopt,
            .placement = {starting_coords, starting_coords, requested_size.x, requested_size.y},
        });
        starting_coords += 50;
    }
    void on_flip_window(u32 window_id, u32 surface_id) override {
        for (auto& win : m_windows) {
            if (win.id != window_id) continue;
            for (auto& surf : m_surfaces) {
                if (surf.first != surface_id) continue;
                win.current_surface_id = surface_id;
                win.placement.size = window::Vec(surf.second.width(), surf.second.height());
            }
        }
        dbgln("Cannot flip (invalid ids): window {}, surface {}"_sv, window_id, surface_id);
    }
    void on_begin_window_operation(u32 operation) override {};
    void on_ping_response() override { last_pong_time = current_time; }

    void blit(window::RenderContext& ctx) {
        window::Renderer renderer{ctx, ctx.render_rect()};
        for (auto& win : m_windows) {
            if (win.current_surface_id) {
                for (auto& surf : m_surfaces) {
                    if (surf.first == *win.current_surface_id) {
                        // Let's gooo
                        renderer.paint_bitmap(surf.second, win.placement, {0, 0});
                    }
                }
            }
        }
    }

private:
    bek::vector<bek::pair<u32, window::OwningBitmap>> m_surfaces;
    bek::vector<Window> m_windows;
    u64 last_ping_time{0};
    u64 last_pong_time{1};
};

#define EXPECTED_TRY_MESSAGE(EXPRESSION, FAILURE_MSG)    \
    ({                                                   \
        auto&& _temp = (EXPRESSION);                     \
        if (_temp.has_error()) {                         \
            dbgln(FAILURE_MSG ": {}"_sv, _temp.error()); \
            return _temp.error();                        \
        }                                                \
        _temp.release_value();                           \
    })

core::expected<bek::string> get_device_address(DeviceProtocol protocol) {
    auto devices = core::Device::get_devices(protocol);
    if (devices.size() != 1) {
        dbgln("Found {} devices - need one."_sv, devices.size());
        return EFAIL;
    }
    return devices[0].name;
}

inline constexpr uSize FREQUENCY = 60;
inline constexpr uSize NS_PER_FRAME = 1'000'000'000 / FREQUENCY;
inline constexpr uSize BAD_FRAME_LENGTH = NS_PER_FRAME / 2 * 3;

core::expected<int> run() {
    auto fb_address = EXPECTED_TRY_MESSAGE(get_device_address(DeviceProtocol::FramebufferProvider),
                                           "Could not find framebuffer provider");
    auto fb =
        EXPECTED_TRY_MESSAGE(FramebufferDevice::create(fb_address.view()), "Failed to initialise graphics device");
    window::Rect confinement{{0, 0}, {fb->width(), fb->height()}};

    auto kb_address = EXPECTED_TRY_MESSAGE(get_device_address(DeviceProtocol::Keyboard), "Could not find keyboard");
    auto kb = EXPECTED_TRY_MESSAGE(KbdDevice::create(kb_address.view()), "Failed to initialise keyboard device");

    auto mouse_address = EXPECTED_TRY_MESSAGE(get_device_address(DeviceProtocol::Mouse), "Could not find mouse");
    auto mouse = EXPECTED_TRY_MESSAGE(MouseDevice::create(mouse_address.view(), confinement),
                                      "Failed to initialise mouse device");

    window::RenderContext ctx =
        window::RenderContext::create(fb->framebuffer(), fb->stride(), fb->width(), fb->height());

    bek::vector<bek::own_ptr<WindowServerConnection>> connections;
    dbgln("Advertising 'windowserver'"_sv);
    auto advertise_fd =
        EXPECTED_TRY_MESSAGE(core::syscall::interlink::advertise("windowserver"_sv, 0), "advertise() failed");

    starting_coords = 50;
    u64 last_blit = core::syscall::get_ticks();
    window::Vec last_mouse_position{};

    // Mainloop
    while (true) {
        current_time = core::syscall::get_ticks();
        // First, we try to accept any calls.
        auto accept_res = core::syscall::interlink::accept(advertise_fd, 0, false);
        if (accept_res.has_error() && accept_res.error() != EAGAIN) {
            dbgln("accept() failed: "_sv, accept_res.error());
            return accept_res.error();
        } else if (accept_res.has_value()) {
            dbgln("accept() succeeded."_sv);
            connections.push_back(bek::make_own<WindowServerConnection>(accept_res.value()));
        }

        // Next, we handle any messages
        for (auto& connection : connections) {
            auto res = connection->poll();
            if (res != ESUCCESS) {
                dbgln("Poll connection failed: {}"_sv, res);
                return res;
            }
        }

        mouse->update();
        // Next, we blit!
        if (current_time - last_blit > NS_PER_FRAME) {
            window::Renderer renderer{ctx, ctx.render_rect()};
            auto old_mouse_rect = window::Rect{last_mouse_position, {50, 50}}.intersection(ctx.render_rect());
            renderer.paint_rect(0, old_mouse_rect);

            for (auto& connection : connections) {
                connection->blit(ctx);
            }
            last_mouse_position = mouse->position();
            auto new_mouse_rect = window::Rect{last_mouse_position, {50, 50}}.intersection(ctx.render_rect());
            renderer.paint_rect(mouse->is_clicked(0) ? window::BLUE : window::RED, new_mouse_rect);
            fb->flush();
            current_time = core::syscall::get_ticks();
            if (current_time - last_blit > BAD_FRAME_LENGTH) {
                dbgln("Bad frame length: {}"_sv, current_time - last_blit);
            }
            last_blit = current_time;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    auto run_result = run();
    if (run_result.has_error()) {
        return -run_result.error();
    } else {
        return 0;
    }
}