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

#ifndef BEKOS_VIRTIO_REGS_H
#define BEKOS_VIRTIO_REGS_H

#include "peripherals/bitfield_macros.h"

namespace virtio {

struct BaseRegs {
    BEK_REGISTER_RO(MAGIC, u32, 0x00)
    BEK_REGISTER_RO(VERSION, u32, 0x04)
    BEK_REGISTER_RO(DEVICE_ID, u32, 0x08)
    BEK_REGISTER_RO(VENDOR_ID, u32, 0x0C)
    BEK_REGISTER_RO(DEV_FEATURES, u32, 0x10)
    BEK_REGISTER(DEV_FEATURES_SEL, u32, 0x14)
    BEK_REGISTER(DRV_FEATURES, u32, 0x20)
    BEK_REGISTER(DRV_FEATURES_SEL, u32, 0x24)
    BEK_REGISTER(QUEUE_SEL, u32, 0x30)
    BEK_REGISTER_RO(QUEUE_NUM_MAX, u32, 0x34)
    BEK_REGISTER(QUEUE_NUM, u32, 0x38)
    BEK_REGISTER(QUEUE_READY, u32, 0x44)
    BEK_REGISTER(QUEUE_NOTIFY, u32, 0x50)
    BEK_REGISTER_RO(INTERRUPT_STATUS, u32, 0x60)
    BEK_REGISTER(INTERRUPT_ACK, u32, 0x64)
    BEK_REGISTER(STATUS, u32, 0x70)
    BEK_REGISTER(QUEUE_DESC, u64, 0x80)
    BEK_REGISTER(QUEUE_DRV, u64, 0x90)
    BEK_REGISTER(QUEUE_DEV, u64, 0xa0)
    BEK_REGISTER(SHM_SEL, u32, 0xac)
    BEK_REGISTER_RO(SHM_LEN, u64, 0xb0)
    BEK_REGISTER_RO(SHM_BASE, u64, 0xb8)
    BEK_REGISTER(QUEUE_RESET, u32, 0xc0)
    BEK_REGISTER_RO(CONFIG_GENERATION, u32, 0xfc)

    // Status
    BEK_BIT_ACCESSOR_RW1S(sts_ack, STATUS, 0)
    BEK_BIT_ACCESSOR_RW1S(sts_driver, STATUS, 1)
    BEK_BIT_ACCESSOR_RW1S(sts_drv_ok, STATUS, 2)
    BEK_BIT_ACCESSOR_RW1S(sts_feat_ok, STATUS, 3)
    BEK_BIT_ACCESSOR_RW1S(sts_needs_rst, STATUS, 6)
    BEK_BIT_ACCESSOR_RW1S(sts_failed, STATUS, 7)

    // Not technically pcie. We need to ensure that u64 r/w are done in two u32 r/w.
    mem::PCIeDeviceArea base;
};

enum StandardFeatureBits : u64 {
    FEAT_INDIRECT        = BEK_BIT(28),
    FEAT_EVENT_IDX       = BEK_BIT(29),
    FEAT_VERSION_1       = BEK_BIT(32),
    FEAT_ACCESS_PLATFORM = BEK_BIT(33),
    FEAT_RING_PACKED     = BEK_BIT(34),
    FEAT_IN_ORDER        = BEK_BIT(35)
};

constexpr inline u64 standard_feat_supported =
    FEAT_INDIRECT | FEAT_VERSION_1 | FEAT_ACCESS_PLATFORM | FEAT_RING_PACKED;
constexpr inline u64 standard_feat_required = FEAT_VERSION_1;

struct BlkDevRegs {
    enum Features {
        FEAT_SIZE_MAX     = BEK_BIT(1),
        FEAT_SEG_MAX      = BEK_BIT(2),
        FEAT_GEOMETRY     = BEK_BIT(4),
        FEAT_RO           = BEK_BIT(5),
        FEAT_BLK_SIZE     = BEK_BIT(6),
        FEAT_FLUSH        = BEK_BIT(9),
        FEAT_TOPOLOGY     = BEK_BIT(10),
        FEAT_CONFIG_WCE   = BEK_BIT(11),
        FEAT_MQ           = BEK_BIT(12),
        FEAT_DISCARD      = BEK_BIT(13),
        FEAT_WRITE_ZEROES = BEK_BIT(14),
        FEAT_LIFETIME     = BEK_BIT(15),
        FEAT_SECURE_ERASE = BEK_BIT(16),
    };

    BEK_REGISTER_RO(capacity, u64, 0x00)
    BEK_REGISTER_RO(size_max, u32, 0x08)
    BEK_REGISTER_RO(seg_max, u32, 0x0C)
    BEK_REGISTER_RO(geo_cylinders, u16, 0x10)
    BEK_REGISTER_RO(geo_heads, u8, 0x12)
    BEK_REGISTER_RO(geo_sectors, u8, 0x13)
    BEK_REGISTER_RO(blk_size, u32, 0x14)
    BEK_REGISTER_RO(topo_phys_block_exp, u8, 0x18)
    BEK_REGISTER_RO(topo_align_off, u8, 0x19)
    BEK_REGISTER_RO(topo_min_io_size, u16, 0x1A)
    BEK_REGISTER_RO(topo_opt_io_size, u32, 0x1C)
    BEK_REGISTER_RO(WRITEBACK, u8, 0x20)
    BEK_REGISTER_RO(num_queues, u16, 0x22)
    BEK_REGISTER_RO(max_discard_sec, u32, 0x24)
    BEK_REGISTER_RO(max_discard_seq, u32, 0x28)
    BEK_REGISTER_RO(discard_sec_align, u32, 0x2C)
    BEK_REGISTER_RO(max_write_0_sec, u32, 0x30)
    BEK_REGISTER_RO(max_write_0_seq, u32, 0x34)
    BEK_REGISTER_RO(max_zeroes_may_unmap, u8, 0x38)
    BEK_REGISTER_RO(max_sec_erase_sec, u32, 0x3C)
    BEK_REGISTER_RO(max_sec_erase_seq, u32, 0x40)
    BEK_REGISTER_RO(sec_erase_sec_align, u32, 0x44)

    mem::PCIeDeviceArea base;
};

}  // namespace virtio
#endif  // BEKOS_VIRTIO_REGS_H
