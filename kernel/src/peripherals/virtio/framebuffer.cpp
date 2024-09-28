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

#include <peripherals/virtio/framebuffer.h>

#include "bek/array.h"
#include "library/debug.h"
#include "mm/barriers.h"

using DBG = DebugScope<"VirtGpu", DebugLevel::WARN>;
using namespace protocol::fb;

struct [[gnu::packed]] ControlHeader {
    enum Type : u32 {
        /* 2d commands */
        CMD_GET_DISPLAY_INFO = 0x0100,
        CMD_RESOURCE_CREATE_2D,
        CMD_RESOURCE_UNREF,
        CMD_SET_SCANOUT,
        CMD_RESOURCE_FLUSH,
        CMD_TRANSFER_TO_HOST_2D,
        CMD_RESOURCE_ATTACH_BACKING,
        CMD_RESOURCE_DETACH_BACKING,
        CMD_GET_CAPSET_INFO,
        CMD_GET_CAPSET,
        CMD_GET_EDID,
        CMD_RESOURCE_ASSIGN_UUID,
        CMD_RESOURCE_CREATE_BLOB,
        CMD_SET_SCANOUT_BLOB,
        /* 3d commands */
        CMD_CTX_CREATE = 0x0200,
        CMD_CTX_DESTROY,
        CMD_CTX_ATTACH_RESOURCE,
        CMD_CTX_DETACH_RESOURCE,
        CMD_RESOURCE_CREATE_3D,
        CMD_TRANSFER_TO_HOST_3D,
        CMD_TRANSFER_FROM_HOST_3D,
        CMD_SUBMIT_3D,
        CMD_RESOURCE_MAP_BLOB,
        CMD_RESOURCE_UNMAP_BLOB,
        /* cursor commands */
        CMD_UPDATE_CURSOR = 0x0300,
        CMD_MOVE_CURSOR,
        /* success responses */
        RESP_OK_NODATA = 0x1100,
        RESP_OK_DISPLAY_INFO,
        RESP_OK_CAPSET_INFO,
        RESP_OK_CAPSET,
        RESP_OK_EDID,
        RESP_OK_RESOURCE_UUID,
        RESP_OK_MAP_INFO,
        /* error responses */
        RESP_ERR_UNSPEC = 0x1200,
        RESP_ERR_OUT_OF_MEMORY,
        RESP_ERR_INVALID_SCANOUT_ID,
        RESP_ERR_INVALID_RESOURCE_ID,
        RESP_ERR_INVALID_CONTEXT_ID,
        RESP_ERR_INVALID_PARAMETER,
    } type;
    u32 flags;
    u64 fence_id;
    u32 ctx_id;
    u8 ring_idx;
    u8 _padding[3];
};

struct GpuRect {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
};
static_assert(sizeof(GpuRect) == 4 * 4 && alignof(GpuRect) == 4);

struct GpuDisplayInfo {
    ControlHeader header;
    struct SingleInfo {
        GpuRect rect;
        u32 enabled;
        u32 flags;
    } displays[16];
};

struct GpuResourceCreate {
    ControlHeader header;
    u32 resource_id;
    enum ResourceColourFormat : u32 {
        B8G8R8A8_UNORM = 1,
        B8G8R8X8_UNORM = 2,
        A8R8G8B8_UNORM = 3,
        X8R8G8B8_UNORM = 4,
        R8G8B8A8_UNORM = 67,
        X8B8G8R8_UNORM = 68,
        A8B8G8R8_UNORM = 121,
        R8G8B8X8_UNORM = 134,
    } format;
    u32 width;
    u32 height;
};

static_assert(sizeof(GpuResourceCreate) == 4 * (6 + 4));

struct GpuSetScanout {
    ControlHeader header;
    GpuRect rect;
    u32 scanout_id;
    u32 resource_id;
};

struct GpuFlushResource {
    ControlHeader header;
    GpuRect rect;
    u32 resource_id;
    u32 padding;
};

struct GpuTransferToHost {
    ControlHeader header;
    GpuRect rect;
    u64 offset;
    u32 resource_id;
    u32 padding;
};

struct GpuAttachBacking {
    ControlHeader header;
    u32 resource_id;
    u32 number_entries;
    u64 address;
    u32 length;
    u32 padding;
};

static_assert(sizeof(GpuDisplayInfo) == 4 * (6 + 16 * 6));

constexpr ControlHeader basic_control_header(ControlHeader::Type kind) { return {kind, 0, 0, 0, 0, {0, 0, 0}}; }

template <typename T>
bek::array<virtio::TransferElement, 2> basic_command_transfers(const mem::dma_object<T>& data) {
    return {virtio::TransferElement{virtio::TransferElement::OUT, data.get_view()},
            virtio::TransferElement{virtio::TransferElement::IN, data.get_view()}};
}

void bek_basic_format(bek::OutputStream& out, const GpuDisplayInfo::SingleInfo& display) {
    bek::format_to(out, "{}x{} display at ({},{}) with flags {:b} ({})"_sv, display.rect.width, display.rect.height,
                   display.rect.x, display.rect.y, display.flags, display.enabled ? "enabled"_sv : "disabled"_sv);
}

constexpr inline u32 DEFAULT_RESOURCE_ID = 1u;

#define COLOUR_CASES      \
    SWITCH_CASE(R8G8B8A8) \
    SWITCH_CASE(R8G8B8X8) \
    SWITCH_CASE(B8G8R8A8) \
    SWITCH_CASE(B8G8R8X8) \
    SWITCH_CASE(A8R8G8B8) \
    SWITCH_CASE(X8R8G8B8) \
    SWITCH_CASE(A8B8G8R8) \
    SWITCH_CASE(X8B8G8R8)

constexpr ColourFormat colour_format_virtio_to_general(GpuResourceCreate::ResourceColourFormat v) {
    switch (v) {
#define SWITCH_CASE(FORMAT)                 \
    case GpuResourceCreate::FORMAT##_UNORM: \
        return FORMAT;
        COLOUR_CASES
#undef SWITCH_CASE
    }
}

constexpr bek::optional<GpuResourceCreate::ResourceColourFormat> colour_format_general_to_virtio(ColourFormat v) {
    switch (v) {
#define SWITCH_CASE(FORMAT) \
    case FORMAT:            \
        return GpuResourceCreate::FORMAT##_UNORM;
        COLOUR_CASES
#undef SWITCH_CASE
        default:
            return bek::nullopt;
    }
}

void virtio::GraphicsDevice::create_gpu_dev(bek::own_ptr<MMIOTransport> transport) {
    auto features = transport->configure_features(standard_feat_required, standard_feat_supported);
    if (!features) return;

    GpuDevRegs gpu_regs{transport->device_config_area()};
    DBG::dbgln("Number scanouts: {}"_sv, gpu_regs.num_scanouts());

    if (!transport->setup_vqueue(0)) {
        DBG::errln("Could not setup vqueue."_sv);
        return;
    }

    auto data = mem::dma_object<GpuDisplayInfo>{transport->get_dma_pool()};
    data->header = {ControlHeader::CMD_GET_DISPLAY_INFO, 0, 0, 0, 0, {0, 0, 0}};

    TransferElement transfer[2] = {{TransferElement::OUT, data.get_view().subdivide(0, sizeof(ControlHeader))},
                                   {TransferElement::IN, data.get_view()}};

    if (!transport->queue_transfer(
            0, {transfer, 2}, Callback([data = bek::move(data), tp = bek::move(transport)](uSize transferred) mutable {
                if (data->header.type != ControlHeader::RESP_OK_DISPLAY_INFO) {
                    DBG::errln("Failed to get display info."_sv);
                    return;
                }
                DBG::infoln("Display Info:"_sv);
                uSize selected_id = 16;
                for (uSize i = 0; i < 16; i++) {
                    if (data->displays[i].enabled) {
                        DBG::infoln("    Scanout {}: {}"_sv, i, data->displays[i]);

                        selected_id = bek::min(selected_id, i);
                    }
                }
                if (selected_id == 16) {
                    // No valid display
                    DBG::errln("No valid scanout."_sv);
                    return;
                }

                mem::log_kmalloc_usage();
                auto* display = new GraphicsDevice(bek::move(tp), selected_id, data->displays[selected_id].rect.width,
                                                   data->displays[selected_id].rect.height);
                display->initialise();
                DeviceRegistry::the().register_device("generic.framebuffer.virtio"_sv, bek::adopt_shared(display));
            }))) {
        DBG::errln("Could not schedule transfer."_sv);
    }
}

constexpr uSize framebuffer_size(u32 width, u32 height) { return width * height * 4; }

virtio::GraphicsDevice::GraphicsDevice(bek::own_ptr<MMIOTransport> transport, u32 scanout_id, u32 width, u32 height)
    : m_transport(bek::move(transport)),
      m_scanout_id(scanout_id),
      m_information(height, width, R8G8B8A8, false, true),
      m_framebuffer(m_transport->get_dma_pool().allocate(framebuffer_size(width, height), 1)),
      m_fb_info({{reinterpret_cast<u8*>(m_framebuffer.data())}, m_framebuffer.size()}, 4 * m_information.width,
                bek::adopt_shared(
                    new DeviceBackedRegion({*mem::kernel_virt_to_phys(m_framebuffer.data()), m_framebuffer.size()}))) {}

const FramebufferInfo& virtio::GraphicsDevice::front_buffer() const { return m_fb_info; }

protocol::fb::DisplayInfo virtio::GraphicsDevice::info() const { return m_information; }

bool virtio::GraphicsDevice::initialise() {
    DBG::dbgln("Creating Resource"_sv);
    // Create Resource
    auto create_request = mem::dma_object<GpuResourceCreate>{m_transport->get_dma_pool()};
    *create_request = {basic_control_header(ControlHeader::CMD_RESOURCE_CREATE_2D), DEFAULT_RESOURCE_ID,
                       GpuResourceCreate::R8G8B8A8_UNORM, m_information.width, m_information.height};
    create_request.sync_after_write();
    auto transfers = basic_command_transfers(create_request);
    m_transport->queue_transfer(
        0, transfers.span(), virtio::Callback([this, create_request = bek::move(create_request)](uSize transferred) {
            if (create_request->header.type != ControlHeader::RESP_OK_NODATA) {
                DBG::errln("Create Resource Failed with response {}"_sv, (u32)create_request->header.type);
                return;
            } else {
                DBG::dbgln("Created Resource"_sv);
            }

            // Attach Backing
            auto attach_request = mem::dma_object<GpuAttachBacking>{m_transport->get_dma_pool()};
            *attach_request = {basic_control_header(ControlHeader::CMD_RESOURCE_ATTACH_BACKING),
                               DEFAULT_RESOURCE_ID,
                               1,
                               m_framebuffer.dma_ptr().get(),
                               static_cast<u32>(m_framebuffer.size()),
                               0};
            auto transfers = basic_command_transfers(attach_request);
            if (m_transport->queue_transfer(
                    0, transfers.span(),
                    virtio::Callback([this, attach_request = bek::move(attach_request)](uSize transferred) {
                        if (attach_request->header.type != ControlHeader::RESP_OK_NODATA) {
                            DBG::dbgln("Attach Backing failed with response {}"_sv, (u32)attach_request->header.type);
                            return;
                        }
                        // Set Scanout
                        auto scanout_request = mem::dma_object<GpuSetScanout>{m_transport->get_dma_pool()};
                        *scanout_request = {basic_control_header(ControlHeader::CMD_SET_SCANOUT),
                                            {0, 0, m_information.width, m_information.height},
                                            m_scanout_id,
                                            DEFAULT_RESOURCE_ID};
                        auto transfers = basic_command_transfers(scanout_request);
                        m_transport->queue_transfer(
                            0, transfers.span(),
                            virtio::Callback([scanout_request = bek::move(scanout_request)](uSize transferred) {
                                if (scanout_request->header.type != ControlHeader::RESP_OK_NODATA) {
                                    DBG::errln("Set Scanout failed with response {}"_sv,
                                               (u32)scanout_request->header.type);
                                    return;
                                }
                                DBG::dbgln("Set Scanout Success"_sv);
                            }));
                    }))) {
            }
        }));

    return true;
}
ErrorCode virtio::GraphicsDevice::flush_rect(protocol::fb::Rect r) {
    if (r.x >= m_information.width || r.y >= m_information.height || r.width > m_information.width ||
        r.height > m_information.height || (r.x + r.width) > m_information.width ||
        (r.y + r.height) > m_information.height) {
        // Rectangle is incorrect size.
        DBG::errln("Cannot flush rect of size {}x{} @ ({}, {}) to framebuffer of size {}x{}."_sv, r.width, r.height,
                   r.x, r.y, m_information.width, m_information.height);
        return EINVAL;
    }
    DBG::dbgln("Flushing rect  {}x{} @ ({}, {}) to framebuffer {}x{}."_sv, r.width, r.height, r.x, r.y,
               m_information.width, m_information.height);

    {
        u64 offset_to_start = r.y * m_fb_info.byte_stride +
                              r.x * bek::ceil_div(colour_format_bit_width(m_information.colour_format), 8ul);

        auto transfer_request =
            mem::dma_object<GpuTransferToHost>{m_transport->get_dma_pool(),
                                               {basic_control_header(ControlHeader::CMD_TRANSFER_TO_HOST_2D),
                                                {r.x, r.y, r.width, r.height},
                                                offset_to_start,
                                                DEFAULT_RESOURCE_ID,
                                                0}};

        auto transfers = basic_command_transfers(transfer_request);

        mem::CompletionFlag complete;
        ErrorCode error_waiting = ESUCCESS;
        // Take reference to `transfer_request` because we're doing this synchronously.
        if (!m_transport->queue_transfer(0, transfers.span(), virtio::Callback([&](uSize transferred) {
                                             transfer_request.sync_before_read();
                                             if (transfer_request->header.type != ControlHeader::RESP_OK_NODATA) {
                                                 DBG::errln("CMD_TRANSFER_TO_HOST_2D failed with response {}"_sv,
                                                            (u32)transfer_request->header.type);
                                                 error_waiting = EFAIL;
                                             } else {
                                                 error_waiting = ESUCCESS;
                                             }
                                             complete.set();
                                         }))) {
            DBG::errln("Failed to queue CMD_TRANSFER_TO_HOST_2D"_sv);
            return EFAIL;
        }

        complete.wait();
        if (error_waiting != ESUCCESS) {
            return error_waiting;
        }
    }
    {
        auto flush_request = mem::dma_object<GpuFlushResource>{m_transport->get_dma_pool(),
                                                               {basic_control_header(ControlHeader::CMD_RESOURCE_FLUSH),
                                                                {r.x, r.y, r.width, r.height},
                                                                DEFAULT_RESOURCE_ID,
                                                                0}};

        auto transfers = basic_command_transfers(flush_request);

        mem::CompletionFlag complete;
        ErrorCode error_waiting = ESUCCESS;
        // Take reference to `flush_request` because we're doing this synchronously.
        if (!m_transport->queue_transfer(0, transfers.span(), virtio::Callback([&](uSize transferred) {
                                             flush_request.sync_before_read();
                                             if (flush_request->header.type != ControlHeader::RESP_OK_NODATA) {
                                                 DBG::dbgln("CMD_RESOURCE_FLUSH failed with response {}"_sv,
                                                            (u32)flush_request->header.type);
                                                 error_waiting = EFAIL;
                                             } else {
                                                 error_waiting = ESUCCESS;
                                             }
                                             complete.set();
                                         }))) {
            DBG::errln("Failed to queue CMD_RESOURCE_FLUSH"_sv);
            return EFAIL;
        }

        complete.wait();
        if (error_waiting != ESUCCESS) {
            return error_waiting;
        }
    }
    DBG::dbgln("Flush Completed"_sv);
    return ESUCCESS;
}
expected<DisplayInfo> virtio::GraphicsDevice::try_set_info(const DisplayInfo& info) { return m_information; }
