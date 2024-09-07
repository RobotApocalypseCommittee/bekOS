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

#include "peripherals/virtio/block.h"

#include "filesystem/block_device.h"
#include "library/debug.h"

using DBG = DebugScope<"VirtioBlock", true>;

namespace virtio {
constexpr inline u64 blk_feat_required = standard_feat_required;
constexpr inline u64 blk_feat_supported =
    standard_feat_supported | BlkDevRegs::FEAT_RO | BlkDevRegs::FEAT_MQ;

struct BlockRequest {
    enum Type : u32 {
        IN  = 0,
        OUT = 1,
    } type;
    u32 _reserved;
    u64 sector;
    u8 data[512];
    u8 status;
};

struct [[gnu::packed]] BlockRequestHeader {
    enum Type : u32 {
        IN  = 0,
        OUT = 1,
    } type;
    u32 _reserved;
    u64 sector;
};

void BlockDevice::create_block_dev(bek::own_ptr<MMIOTransport> transport) {
    auto features = transport->configure_features(blk_feat_required, blk_feat_supported);
    if (!features) return;

    BlkDevRegs block_regs{transport->device_config_area()};
    DBG::dbgln("{}Drive: capacity {}KB."_sv, (*features & BlkDevRegs::FEAT_RO) ? "Read-only " : "",
               block_regs.capacity() / 2);

    if (!transport->setup_vqueue(0)) {
        DBG::dbgln("Could not setup vqueue."_sv);
        return;
    }

    auto& registry  = blk::BlockDeviceRegistry::the();
    auto [name, id] = registry.allocate_identifiers("virtio"_sv);

    uSize optimal_size =
        (*features & BlkDevRegs::FEAT_BLK_SIZE) ? block_regs.blk_size() : blk::SECTOR_SIZE;

    auto blk_dev =
        bek::own_ptr(new BlockDevice(bek::move(name), id, bek::move(transport), optimal_size,
                                     block_regs.capacity(), *features & BlkDevRegs::FEAT_RO));
    registry.register_raw_device(bek::move(blk_dev));
}
uSize BlockDevice::logical_block_size() const { return m_logical_block_size; }
bool BlockDevice::is_read_only() const { return m_readonly; }
uSize BlockDevice::capacity() const { return m_capacity_sectors; }
blk::TransferResult BlockDevice::schedule_read(uSize byte_offset, bek::mut_buffer buffer, blk::TransferCallback cb) {
    // First, we round down offset, and calculate number of sectors to transfer
    uSize sector_index = byte_offset / blk::SECTOR_SIZE;
    uSize offset_within_sector = byte_offset - (sector_index * blk::SECTOR_SIZE);
    uSize sector_count = bek::ceil_div(buffer.size() + offset_within_sector, blk::SECTOR_SIZE);
    uSize transfer_size = sector_count * blk::SECTOR_SIZE;

    auto request = m_transport->get_dma_pool().allocate(transfer_size + 17, 8);
    auto& req_header  = request.raw_view().get_at<BlockRequestHeader>(0);
    req_header.sector = sector_index;
    req_header.type   = BlockRequestHeader::IN;

    TransferElement transfer[3]{
        {TransferElement::OUT, request.view().subdivide(0, 16)},
                                {TransferElement::IN, request.view().subdivide(16, transfer_size)},
                                {TransferElement::IN, request.view().subdivide(transfer_size + 16, 1)}};

    if (!m_transport->queue_transfer(
            0, {transfer, 3},
            Callback([req = bek::move(request), buffer, cb = bek::move(cb), transfer_size,
                                               offset_within_sector](uSize transferred) mutable {
                                         if (transferred != 1 + transfer_size) {
                                             DBG::dbgln("Transferred {} bytes!"_sv, transferred);
                    }
                                         auto status = req.raw_view().get_at<u8>(transfer_size + 16);
                                         DBG::dbgln("Status: {:b}"_sv, status);
                                         bek::memcopy(buffer.data(), req.raw_view().data() + 16 + offset_within_sector,
                                                      buffer.size());
                                         cb(status == 0 ? blk::TransferResult::Success : blk::TransferResult::Failure);
                }))) {
        DBG::dbgln("Could not setup transfer."_sv);
        return blk::TransferResult::Failure;
    }
    return blk::TransferResult::Success;
}
blk::TransferResult BlockDevice::schedule_write(uSize byte_offset, bek::buffer buffer, blk::TransferCallback cb) {
    return blk::TransferResult::Failure;
}
BlockDevice::BlockDevice(bek::string name, u32 global_id, bek::own_ptr<MMIOTransport> transport,
                         uSize logical_block_size, uSize capacity_sectors, bool readonly)
    : blk::BlockDevice(bek::move(name), global_id),
      m_transport(bek::move(transport)),
      m_logical_block_size(logical_block_size),
      m_capacity_sectors(capacity_sectors),
      m_readonly(readonly) {}
}  // namespace virtio
