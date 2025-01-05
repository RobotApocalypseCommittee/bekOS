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

#include <api/protocols/fb.h>
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

        auto fb_info = protocol::fb::MapMessage{protocol::fb::MapFramebuffer};

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



    void flush() {
        auto flush_msg = protocol::fb::FlushRectMessage{protocol::fb::FlushRect, {0, 0, height(), width()}};
        auto res = core::syscall::message(m_ed, 0, &flush_msg, sizeof(flush_msg));
        if (res.has_error()) {
            core::fprintln(core::stdout, "Failed to flush!"_sv);
        }
    }
};

class WindowServerConnection: public window::WindowServerRaw {
public:
    struct Window {
        u32 id;
        bek::optional<u32> current_surface_id;
        window::Rect placement;
    };
    using WindowServerRaw::WindowServerRaw;

    void on_create_surface(u32 id, window::OwningBitmap region) override {
        // TODO: Replacing surfaces.
        for (auto& surf: m_surfaces) {
            if (surf.first == id) return;
        }
        m_surfaces.push_back(bek::pair{id, bek::move(region)});
        dbgln("Created surface {} ({}x{})"_sv, id, m_surfaces.back().second.width(), m_surfaces.back().second.height());
    }
    void on_create_window(u32 id, window::Rect requested_size) override {
        for (auto& win: m_windows) {
            if (win.id == id) return;
        }
        dbgln("Created window {}"_sv, id);
        m_windows.push_back(Window{
            .id = id,
            .current_surface_id = bek::nullopt,
            .placement = {starting_coords, starting_coords, 100, 100}
        });
        starting_coords += 50;
    }
    void on_flip_window(u32 window_id, u32 surface_id) override {
        for (auto& win: m_windows) {
            if (win.id != window_id) continue;
            for (auto& surf: m_surfaces) {
                if (surf.first != surface_id) continue;
                win.current_surface_id = surface_id;
                win.placement.size = window::Vec(surf.second.width(), surf.second.height());
            }
        }
        dbgln("Cannot flip (invalid ids): window {}, surface {}"_sv, window_id, surface_id);
    }
    void on_begin_window_operation(u32 operation) override;
    void on_ping_response() override {
        last_pong_time = current_time;
    }

    void blit(window::RenderContext& ctx) {
        window::Renderer renderer{ctx, ctx.render_rect()};
        for (auto& win: m_windows) {
            if (win.current_surface_id) {
                for (auto& surf: m_surfaces) {
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
    u64 last_ping_time;
    u64 last_pong_time{1};
};




int main(int argc, char** argv) {
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

    auto font_context = window::create_blank_context();
    window::RenderContext ctx{
        .buffer = fb.framebuffer(),
        .byte_stride = static_cast<u32>(fb.stride()),
        .pixel_width = fb.width(),
        .pixel_height = fb.height(),
        .confinement = window::Rect{{0, 0}, {fb.width(), fb.height()}},
        .font_context = *font_context
    };

    bek::vector<bek::own_ptr<WindowServerConnection>> connections;
    dbgln("Advertising 'windowserver'"_sv);
    auto advertise_res = core::syscall::interlink::advertise("windowserver"_sv, 0);

    if (advertise_res.has_error()) {
        dbgln("advertise() failed: "_sv, advertise_res.error());
        return -advertise_res.error();
    }

    starting_coords = 50;
    u64 last_blit = core::syscall::get_ticks();

    // Mainloop
    while (true) {
        current_time = core::syscall::get_ticks();
        // First, we try to accept any calls.
        auto accept_res = core::syscall::interlink::accept(advertise_res.value(), 0, false);
        if (accept_res.has_error() && accept_res.error() != EAGAIN) {
            dbgln("accept() failed: "_sv, accept_res.error());
            return -accept_res.error();
        } else if (accept_res.has_value()) {
            dbgln("accept() succeeded."_sv);
            connections.push_back(bek::make_own<WindowServerConnection>(accept_res.value()));
        }

        // Next, we handle any messages
        for (auto& connection: connections) {
            auto res = connection->poll();
            if (res != ESUCCESS) {
                dbgln("Poll connection failed: {}"_sv, res);
                return res;
            }
        }

        // Next, we blit!
        if (current_time - last_blit > 500'000'000) {
            for (auto& connection: connections) {
                connection->blit(ctx);
            }
            last_blit = current_time;
        }
    }
}