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

#ifndef BEKOS_VIRTIO_FRAMEBUFFER_H
#define BEKOS_VIRTIO_FRAMEBUFFER_H

#include "bek/own_ptr.h"
#include "peripherals/framebuffer.h"
#include "peripherals/virtio.h"

namespace virtio {

class GraphicsDevice : public FramebufferDevice {
public:
    static void create_gpu_dev(bek::own_ptr<MMIOTransport> transport);

    GraphicsDevice(bek::own_ptr<MMIOTransport> transport, u32 scanout_id, u32 width, u32 height);
    bool initialise();
    const FramebufferInfo& front_buffer() const override;
    ErrorCode flush_rect(protocol::fb::Rect r) override;
    expected<protocol::fb::DisplayInfo> try_set_info(const protocol::fb::DisplayInfo& info) override;
    protocol::fb::DisplayInfo info() const override;

private:
    bek::own_ptr<MMIOTransport> m_transport;
    u32 m_scanout_id;
    protocol::fb::DisplayInfo m_information;
    mem::own_dma_buffer m_framebuffer;
    FramebufferInfo m_fb_info;
};

}  // namespace virtio

#endif  // BEKOS_VIRTIO_FRAMEBUFFER_H
