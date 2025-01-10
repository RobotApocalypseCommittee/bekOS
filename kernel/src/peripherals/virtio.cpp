// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
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

#include "peripherals/virtio.h"

#include "library/debug.h"
#include "mm/dma_utils.h"
#include "mm/memory_manager.h"
#include "peripherals/interrupt_controller.h"
#include "peripherals/virtio/block.h"
#include "peripherals/virtio/framebuffer.h"
#include "peripherals/virtio/virtio_regs.h"

using DBG = DebugScope<"Virtio", DebugLevel::WARN>;

namespace virtio {

constexpr uSize magic_number = 0x74726976;

using probe_t                     = void (*)(bek::own_ptr<MMIOTransport>);
constexpr probe_t driver_probes[] = {nullptr,
                                     nullptr,
                                     BlockDevice::create_block_dev,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     GraphicsDevice::create_gpu_dev};

struct SplitVQDescriptor {
    enum Flags : u16 {
        NONE     = 0,
        NEXT     = 1,
        WRITE    = 2,
        INDIRECT = 4,
    };
    u64 address;
    u32 length;
    u16 flags;
    u16 next;
};
static_assert(sizeof(SplitVQDescriptor) == 16);

struct SplitVQAvailableHeader {
    enum : u16 {
        NO_FLAGS     = 0,
        NO_INTERRUPT = 1,
    } flags;
    u16 idx;
    u16 entries[0];
};
static_assert(sizeof(SplitVQAvailableHeader) == 4);
constexpr uSize split_vq_available_size(u32 queue_num) { return 6 + 2 * queue_num; }

struct SplitVQUsedEntry {
    u32 id;
    u32 length;
};
static_assert(sizeof(SplitVQUsedEntry) == 8);

struct SplitVQUsedHeader {
    enum : u16 { NO_NOTIFY = 1 } flags;
    u16 idx;
    SplitVQUsedEntry entries[0];
};
static_assert(sizeof(SplitVQUsedHeader) == 4);
constexpr uSize split_vq_used_size(u32 queue_num) { return 6 + 8 * queue_num; }

struct SplitVQ {
    SplitVQ(mem::dma_pool& pool, u32 queue_num, u32 queue_idx)
        : descriptors(pool, queue_num, 16),
          available(pool.allocate(split_vq_available_size(queue_num), 2)),
          used(pool.allocate(split_vq_used_size(queue_num), 4)),
          next_free_desc{0},
          id(queue_idx) {
        auto& av_hdr = available.raw_view().get_at<SplitVQAvailableHeader>(0);
        av_hdr.flags = SplitVQAvailableHeader::NO_FLAGS;
        av_hdr.idx   = 0;

        for (u32 i = 0; i < queue_num; i++) {
            // Free descriptors point to next free. next = queue_num -> end.
            descriptors[i].next = i + 1;
        }
        free_descriptors = queue_num;
    }

    bek::optional<u32> allocate_descriptor() {
        if (next_free_desc < descriptors.size()) {
            free_descriptors--;
            auto idx = next_free_desc;
            // Important - replace with invalid value.
            next_free_desc = bek::exchange(descriptors[idx].next, static_cast<u16>(descriptors.size()));
            return idx;
        } else {
            return {};
        }
    }

    void place_available(u16 index) {
        auto& hdr = available.raw_view().get_at<SplitVQAvailableHeader>(0);
        hdr.entries[hdr.idx % descriptors.size()] = index;
        hdr.idx += 1;
    }

    void process_used() {
        auto& hdr = used.raw_view().get_at<SplitVQUsedHeader>(0);
        while (last_seen_used_idx != hdr.idx) {
            auto& used_entry = hdr.entries[last_seen_used_idx % descriptors.size()];
            auto res = callbacks.extract(used_entry.id);

            // Free Descriptors
            u64 descriptors_to_free = 1;
            auto desc_idx = used_entry.id;
            while (descriptors[desc_idx].flags & SplitVQDescriptor::NEXT) {
                desc_idx = descriptors[desc_idx].next;
                descriptors_to_free++;
            }
            // Add to front of chain.
            descriptors[desc_idx].next = bek::exchange(next_free_desc, used_entry.id);
            free_descriptors += descriptors_to_free;

            // Here, we assume it will wrap appropriately.
            last_seen_used_idx++;

            // If there was a callback, and the callback was not empty.
            if (res && *res) {
                (*res)(used_entry.length);
            }
        }
    }

    mem::dma_array<SplitVQDescriptor> descriptors;
    mem::own_dma_buffer available;
    mem::own_dma_buffer used;
    bek::hashtable<u16, Callback> callbacks;
    u32 next_free_desc;
    u32 id;
    u32 free_descriptors;
    u32 last_seen_used_idx{};
};

dev_tree::DevStatus MMIOTransport::probe_devtree(dev_tree::Node& node, dev_tree::device_tree& tree,
                                                 dev_tree::probe_ctx& ctx) {
    if (!(node.compatible.size() && node.compatible[0] == "virtio,mmio"_sv))
        return dev_tree::DevStatus::Unrecognised;

    // Look for interrupt parent.
    auto int_parent_phandle = dev_tree::get_inheritable_property_u32(node, "interrupt-parent"_sv);
    if (!int_parent_phandle) {
        DBG::errln("Could not get interrupt-parent phandle."_sv);
        return dev_tree::DevStatus::Failure;
    }
    auto [p_int_parent, result_status] = dev_tree::get_node_by_phandle(tree, *int_parent_phandle);
    if (!p_int_parent) {
        return result_status;
    }
    auto* interrupt_parent = Device::as<InterruptController>(p_int_parent->attached_device.get());
    ASSERT(interrupt_parent);

    auto int_buf = node.get_property("interrupts"_sv);
    if (!int_buf) {
        DBG::errln("Could not get interrupts."_sv);
        return dev_tree::DevStatus::Failure;
    }
    auto int_handle = interrupt_parent->register_interrupt(*int_buf);
    if (!int_handle) {
        DBG::errln("Could not get interrupt handle."_sv);
        return dev_tree::DevStatus::Failure;
    }

    auto regs = get_regions_from_reg(node);
    if (regs.size() != 1) {
        DBG::errln("Fail: regs must have one region."_sv);
        return dev_tree::DevStatus::Failure;
    }

    auto dma_mapping = dev_tree::get_dma_to_phys_ranges(node);

    mem::PCIeDeviceArea area = mem::MemoryManager::the().map_for_io(regs[0]);
    BaseRegs registers{area};
    if (registers.MAGIC() != magic_number) {
        DBG::errln("Fail: Magic Incorrect."_sv);
        return dev_tree::DevStatus::Failure;
    }

    if (registers.VERSION() != 2) {
        DBG::errln("Fail: Version {} unsupported."_sv, registers.VERSION());
        return dev_tree::DevStatus::Failure;
    }

    if (registers.DEVICE_ID() == 0) {
        DBG::infoln("Fail: DeviceID is 0 (unrecognised)"_sv);
        return dev_tree::DevStatus::Unrecognised;
    }

    // Reset Device
    registers.STATUS(0);
    while (registers.STATUS() != 0) {
    }

    // Acknowledge
    registers.set_sts_ack();

    auto transport = bek::make_own<MMIOTransport>(registers, bek::move(dma_mapping));

    auto dev_id = registers.DEVICE_ID();
    if (dev_id >= (sizeof(driver_probes) / sizeof(*driver_probes)) ||
        driver_probes[dev_id] == nullptr) {
        DBG::errln("Unsupported virtio device id {}."_sv, dev_id);
        registers.set_sts_failed();
        return dev_tree::DevStatus::Failure;
    }

    int_handle.register_handler(
        [transport_p = transport.get()]() { transport_p->on_notification(); });
    int_handle.enable();

    // TODO: Success or failure condition. Attach device to devtree node.
    driver_probes[dev_id](bek::move(transport));
    return dev_tree::DevStatus::Success;
}
bek::optional<u64> MMIOTransport::configure_features(u64 required, u64 supported) {
    // Read features.
    m_regs.DEV_FEATURES_SEL(1);
    u64 device_feats = m_regs.DEV_FEATURES();
    device_feats <<= 32;

    m_regs.DEV_FEATURES_SEL(0);
    device_feats |= m_regs.DEV_FEATURES();

    // Ensure required features are present.
    if ((device_feats & required) != required) {
        DBG::errln("Fail: Required features not present. Difference: {:b}"_sv, (device_feats & required) ^ required);
        m_regs.set_sts_failed();
        return {};
    }

    // Filter out unsupported features.
    device_feats &= supported;

    // Write features
    m_regs.DRV_FEATURES_SEL(1);
    m_regs.DRV_FEATURES(device_feats >> 32);

    m_regs.DRV_FEATURES_SEL(0);
    m_regs.DRV_FEATURES(device_feats & 0xFFFFFFFF);

    // Set OK
    m_regs.set_sts_feat_ok();

    if (!m_regs.get_sts_feat_ok()) {
        DBG::errln("Device failed to accept subset of features. Requested: {:b}"_sv, device_feats);
        m_regs.set_sts_failed();
        return {};
    }

    return device_feats;
}
bool MMIOTransport::setup_vqueue(u32 queue_idx) {
    constexpr u32 sensible_max_queue_num = 32;

    m_regs.QUEUE_SEL(queue_idx);
    if (m_regs.QUEUE_READY()) {
        DBG::errln("Err: Queue {} is already ready."_sv, queue_idx);
        return false;
    }
    auto max_num = m_regs.QUEUE_NUM_MAX();
    if (max_num == 0) {
        DBG::errln("Err: Queue {} is not available."_sv, queue_idx);
        return false;
    }

    // We're going to choose a smaller size sensibly.
    if (max_num > sensible_max_queue_num) {
        DBG::infoln("Choosing queue size {} instead of max {}."_sv, sensible_max_queue_num, max_num);
        max_num = sensible_max_queue_num;
    }

    m_queues.push_back(bek::make_own<SplitVQ>(m_pool, max_num, queue_idx));
    auto& queue = *m_queues.back();

    m_regs.QUEUE_NUM(max_num);
    m_regs.QUEUE_DESC(queue.descriptors.dma_ptr().get());
    m_regs.QUEUE_DRV(queue.available.dma_ptr().get());
    m_regs.QUEUE_DEV(queue.used.dma_ptr().get());
    m_regs.QUEUE_READY(1);

    return true;
}
mem::PCIeDeviceArea MMIOTransport::device_config_area() const {
    return (m_regs.base.size() > 0x100) ? m_regs.base.subdivide(0x100, m_regs.base.size() - 0x100)
                                        : mem::PCIeDeviceArea{mem::DeviceArea{0, 0, 0}};
}
bool MMIOTransport::queue_transfer(u32 queue_idx, bek::span<TransferElement> elements, Callback cb) {
    VERIFY(elements.size());
    m_regs.QUEUE_SEL(queue_idx);
    if (m_regs.QUEUE_READY() != 1) {
        DBG::errln("Err: Queue {} is not ready for transfers."_sv, queue_idx);
        return false;
    }
    SplitVQ* queue = nullptr;
    for (auto& vqueue : m_queues) {
        if (vqueue->id == queue_idx) {
            queue = vqueue.get();
        }
    }
    if (!queue) {
        DBG::errln("Err: Queue {} is not registered with transport."_sv, queue_idx);
        return false;
    }

    if (queue->free_descriptors < elements.size()) {
        DBG::errln("Err: Transfer requires {} descriptors, and only {} free in queue {}."_sv, elements.size(),
                   queue->free_descriptors, queue_idx);
        return false;
    }

    SplitVQDescriptor* last_desc = nullptr;
    u16 start_idx = 0;
    for (auto& element : elements) {
        if (auto idx = queue->allocate_descriptor()) {
            if (last_desc) {
                last_desc->next = *idx;
                last_desc->flags |= SplitVQDescriptor::NEXT;
            } else {
                start_idx = *idx;
            }
            auto& desc = queue->descriptors[*idx];
            desc.address = element.data.dma_ptr().get();
            desc.length = element.data.size();
            desc.flags = (element.direction == TransferElement::OUT) ? 0 : SplitVQDescriptor::WRITE;
            last_desc = &desc;
        } else {
            // This is very unlikely (actually unreachable).
            DBG::errln("Err: Could not allocate descriptor for queue {}."_sv, queue_idx);
            return false;
        }
    }

    auto res = queue->callbacks.insert({start_idx, bek::move(cb)});
    VERIFY(res.second);

    // Now, we place on available ring.
    queue->place_available(start_idx);
    m_regs.QUEUE_NOTIFY(queue_idx);

    return true;
}
MMIOTransport::~MMIOTransport() = default;
void MMIOTransport::on_notification() {
    if (m_regs.INTERRUPT_STATUS() & 1) {
        // Used buffer.
        m_regs.INTERRUPT_ACK(m_regs.INTERRUPT_STATUS());

        for (auto& queue : m_queues) {
            queue->process_used();
        }
    }
}
MMIOTransport::MMIOTransport(virtio::BaseRegs regs, bek::vector<dev_tree::range_t> dma_mappings)
    : m_regs(regs), m_pool(bek::move(dma_mappings)) {}
bool MMIOTransport::blocking_transfer(u32 queue_idx, bek::span<TransferElement> elements) {
    volatile bool complete = false;
    auto res = queue_transfer(queue_idx, elements, [&](uSize transferred) {
        DBG::dbgln("Callback!"_sv);
        complete = true;
    });
    if (!res) return false;
    while (!complete) {
    }
    return true;
}

}  // namespace virtio
