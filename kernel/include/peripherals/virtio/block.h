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

#ifndef BEKOS_VIRTIO_BLOCK_H
#define BEKOS_VIRTIO_BLOCK_H

#include "filesystem/block_device.h"
#include "peripherals/virtio.h"

namespace virtio {

class BlockDevice : public blk::BlockDevice {
public:
    BlockDevice(bek::string name, u32 global_id, bek::own_ptr<MMIOTransport> transport,
                uSize logical_block_size, uSize capacity_sectors, bool readonly);

    static void create_block_dev(bek::own_ptr<MMIOTransport> transport);

    uSize logical_block_size() const override;
    bool is_read_only() const override;
    uSize capacity() const override;
    blk::TransferResult schedule_read(uSize sector_index, bek::mut_buffer buffer,
                                      blk::TransferCallback cb) override;
    blk::TransferResult schedule_write(uSize sector_index, bek::buffer buffer,
                                       blk::TransferCallback cb) override;

private:
    bek::own_ptr<MMIOTransport> m_transport;
    uSize m_logical_block_size;
    uSize m_capacity_sectors;
    bool m_readonly;
};

}  // namespace virtio

#endif  // BEKOS_VIRTIO_BLOCK_H
