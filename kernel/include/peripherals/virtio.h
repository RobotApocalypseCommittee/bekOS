/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_VIRTIO_H
#define BEKOS_VIRTIO_H

#include "device_tree.h"
#include "library/function.h"
#include "mm/areas.h"
#include "mm/dma_utils.h"
#include "peripherals/virtio/virtio_regs.h"

namespace virtio {

struct SplitVQ;

struct TransferElement {
    enum Direction {
        IN,
        OUT,
    } direction;
    mem::dma_buffer data;
};

using Callback = bek::function<void(uSize), false, true>;

class MMIOTransport {
public:
    explicit MMIOTransport(virtio::BaseRegs regs, bek::vector<dev_tree::range_t> dma_mappings)
        : m_regs(regs), m_pool(bek::move(dma_mappings)) {}
    static dev_tree::DevStatus probe_devtree(dev_tree::Node& node, dev_tree::device_tree&);

    bek::optional<u64> configure_features(u64 required, u64 supported);
    mem::PCIeDeviceArea device_config_area() const;

    bool setup_vqueue(u32 queue_idx);
    bool queue_transfer(u32 queue_idx, bek::span<TransferElement> elements, Callback cb);

    mem::dma_pool& get_dma_pool() { return m_pool; }

private:
    void on_notification();

    BaseRegs m_regs;
    bek::vector<bek::own_ptr<SplitVQ>> m_queues;
    mem::MappedDmaPool m_pool;
};

}  // namespace virtio

#endif  // BEKOS_VIRTIO_H
