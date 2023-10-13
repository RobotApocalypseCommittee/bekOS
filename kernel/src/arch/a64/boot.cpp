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

#include "arch/a64/memory_constants.h"
#include "interrupts/int_ctrl.h"
#include "library/array.h"
#include "library/debug.h"
#include "mm/memory_manager.h"
#include "peripherals/arm_gic.h"
#include "peripherals/clock.h"
#include "peripherals/device_tree.h"
#include "peripherals/interrupt_controller.h"
#include "peripherals/pcie.h"
#include "peripherals/uart.h"
#include "peripherals/virtio.h"

static constexpr uPtr qemu_pl011_address = VA_START + 0x900'0000;
static constexpr uPtr qemu_clock_freq    = 0x16e3600;

using PROBE_DBG = DebugScope<"Probe", true>;

dev_tree::DevStatus probe_node(dev_tree::Node& node, dev_tree::device_tree& tree);

dev_tree::DevStatus probe_simple_bus(dev_tree::Node& node, dev_tree::device_tree& tree) {
    if (node.compatible.size() == 0) return dev_tree::DevStatus::Unrecognised;
    if (!(node.compatible[0] == "simple-bus"_sv || node.compatible[0] == "linux,dummy-virt"_sv))
        return dev_tree::DevStatus::Unrecognised;
    for (auto& child : node.children) {
        probe_node(*child, tree);
    }
    return dev_tree::DevStatus::Success;
}

constexpr const inline bek::array standard_probes{
    probe_simple_bus, FixedClock::probe_devtree, PL011::probe_devtree,  pcie::Controller::probe_pcie_host,
    ArmGIC::probe_devtree, virtio::MMIOTransport::probe_devtree};

dev_tree::DevStatus probe_node(dev_tree::Node& node, dev_tree::device_tree& tree) {
    for (auto probe_fn : standard_probes) {
        auto res = probe_fn(node, tree);
        if (res != dev_tree::DevStatus::Unrecognised) {
            node.nodeStatus = res;

            if (res == dev_tree::DevStatus::Waiting) {
                PROBE_DBG::dbgln("Device {} Waiting."_sv, node.name);
                tree.waiting.push_back(&node);
            } else if (res == dev_tree::DevStatus::Success) {
                PROBE_DBG::dbgln("Device {} Success."_sv, node.name);
            } else if (res == dev_tree::DevStatus::Failure) {
                PROBE_DBG::dbgln("Device {} Failure."_sv, node.name);
            }

            return res;
        }
    }

    PROBE_DBG::dbgln("Device {} Unrecognised."_sv, node.name);

    node.nodeStatus = dev_tree::DevStatus::Unrecognised;
    return dev_tree::DevStatus::Unrecognised;
}

bek::pair<mem::PhysicalRegion, bek::buffer> get_devicetree_regions(uPtr device_tree_address) {
    bek::buffer temp{reinterpret_cast<const char*>(device_tree_address), 8};
    uSize size = temp.get_at_be<u32>(4);
    bek::buffer buf{reinterpret_cast<const char*>(device_tree_address), size};
    // Absolute disaster if not mapped to physical.
    auto phys_ptr = mem::kernel_virt_to_phys(reinterpret_cast<void*>(device_tree_address))->get();
    auto bottom_phys_ptr = bek::align_down(phys_ptr, SIZE_2M);
    auto top_phys_ptr    = bek::align_up(phys_ptr + size, SIZE_2M);
    mem::PhysicalRegion region{{bottom_phys_ptr}, top_phys_ptr - bottom_phys_ptr};
    return {region, buf};
}

extern u8 __kernel_start;
extern u8 __kernel_end;

mem::PhysicalRegion get_kernel_region() {
    auto phys_ptr = *mem::kernel_virt_to_phys(&__kernel_start);
    auto size     = &__kernel_end - &__kernel_start;
    return {phys_ptr, static_cast<uSize>(size)};
}

extern "C" {

// ABI stuff
void* __dso_handle;

/// These are no-op for now - I'm assuming destructors don't need to run when system stops
int __cxa_atexit(void (*destructor)(void*), void* arg, void* dso) {
    (void)destructor;
    (void)arg;
    (void)dso;
    return 0;
}
void __cxa_finalize(void*) {}

uPtr g_current_embedded_table_phys;
}

InterruptController* global_intc = nullptr;
PL011* serial                    = nullptr;
bek::OutputStream* debug_stream  = nullptr;

using DBG = DebugScope<"Kern", true>;

extern "C" [[noreturn]] void kernel_boot(u64 dev_tree_address) {
    // Placeholder - temporary
    PL011 uart{qemu_pl011_address, qemu_clock_freq};
    serial       = &uart;
    debug_stream = &uart;
    uart.write("Hello Kernel World!\n"_sv);
    set_vector_table();

    mem::initialise_kmalloc();

    auto [dtb_phys_region, dtb_buffer] = get_devicetree_regions(dev_tree_address);
    auto dtb                           = dev_tree::read_dtb(dtb_buffer);

    auto reserved_regions = dev_tree::get_reserved_regions(dtb);
    reserved_regions.push_back(dtb_phys_region);
    reserved_regions.push_back(get_kernel_region());

    auto memory_space =
        mem::process_memory_regions(dev_tree::get_memory_regions(dtb), reserved_regions);
    DBG::dbgln("Memory Space:"_sv);
    for (auto& region : memory_space) {
        DBG::dbgln("    {}"_sv, region);
    }

    mem::MemoryManager::initialise(
        memory_space, static_cast<u8*>(mem::kernel_phys_to_virt({g_current_embedded_table_phys})));

    auto& node = *dtb.root_node;
    DBG::dbgln("Read Device Tree. Probing."_sv);
    probe_node(node, dtb);
    for (int i = 1; i < 10; i++) {
        if (dtb.waiting.size()) {
            DBG::dbgln("Clearing {} waiting nodes."_sv, dtb.waiting.size());
            auto vec = bek::move(dtb.waiting);
            for (auto node : vec) {
                probe_node(*node, dtb);
            }
        } else {
            DBG::dbgln("All Nodes Probed."_sv);
            break;
        }
    }
    if (auto int_parent = dev_tree::get_property_u32(node, "interrupt-parent"_sv)) {
        auto [intc_node, state] = dev_tree::get_node_by_phandle(dtb, *int_parent);
        if (state == dev_tree::DevStatus::Success) {
            global_intc = Device::as<InterruptController>(intc_node->attached_device.get());
        }
    }

    auto [free_mem, total_mem] = mem::get_kmalloc_usage();
    DBG::dbgln("Memory: {} of {} bytes used ({}%)."_sv, total_mem - free_mem, total_mem,
               ((total_mem - free_mem) * 100) / total_mem);

    enable_interrupts();


    while (true) {
        // enable_interrupts();
        uart.write(uart.getc());
    }
}

extern "C" [[noreturn]] void kernel_main() {
    while (true) {
    }
}