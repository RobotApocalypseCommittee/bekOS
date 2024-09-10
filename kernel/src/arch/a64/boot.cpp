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

#include "arch/a64/memory_constants.h"
#include "filesystem/block_device.h"
#include "filesystem/fatfs.h"
#include "interrupts/deferred_calls.h"
#include "interrupts/int_ctrl.h"
#include "library/array.h"
#include "library/byte_format.h"
#include "library/debug.h"
#include "mm/memory_manager.h"
#include "peripherals/arm/arm_gic.h"
#include "peripherals/arm/gentimer.h"
#include "peripherals/clock.h"
#include "peripherals/device_tree.h"
#include "peripherals/interrupt_controller.h"
#include "peripherals/pcie.h"
#include "peripherals/uart.h"
#include "peripherals/virtio.h"
#include "process/process.h"
#include "process/tty.h"

static constexpr uPtr qemu_pl011_address = VA_START + 0x900'0000;
static constexpr uPtr qemu_clock_freq    = 0x16e3600;

using PROBE_DBG = DebugScope<"Probe", true>;

dev_tree::DevStatus probe_simple_bus(dev_tree::Node& node, dev_tree::device_tree& tree, dev_tree::probe_ctx& ctx) {
    if (node.compatible.size() == 0) return dev_tree::DevStatus::Unrecognised;
    if (node.compatible[0] != "simple-bus"_sv) return dev_tree::DevStatus::Unrecognised;
    for (auto& child : node.children) {
        probe_node(*child, tree, ctx);
    }
    return dev_tree::DevStatus::Success;
}

InterruptController* global_intc = nullptr;

dev_tree::DevStatus probe_arm_system(dev_tree::Node& node, dev_tree::device_tree& tree, dev_tree::probe_ctx& ctx) {
    if (node.compatible.size() == 0) return dev_tree::DevStatus::Unrecognised;
    // TODO: This shouldn't really be hardcoded.
    if (node.compatible[0] == "linux,dummy-virt"_sv || node.compatible[0] == "raspberrypi,4-model-b"_sv) {
        for (auto& child : node.children) {
            probe_node(*child, tree, ctx);
        }

        // First, we find the interrupt controller. Hopefully, it's independent!
        if (auto int_parent = dev_tree::get_property_u32(node, "interrupt-parent"_sv)) {
            auto [intc_node, status] = dev_tree::get_node_by_phandle(tree, *int_parent);
            if (status == dev_tree::DevStatus::Success) {
                global_intc = Device::as<InterruptController>(intc_node->attached_device.get());
            } else {
                return dev_tree::DevStatus::Failure;
            }
        } else {
            return dev_tree::DevStatus::Failure;
        }

        // Next, we register the generic timer - should be present!
        DeviceRegistry::the().register_device("generic.timer.arm"_sv, ArmGenericTimer::probe_timer(*global_intc));

        return dev_tree::DevStatus::Success;
    } else {
        return dev_tree::DevStatus::Unrecognised;
    }
}

constexpr const inline bek::array standard_probes{probe_arm_system,
                                                  probe_simple_bus,
                                                  FixedClock::probe_devtree,
                                                  PL011::probe_devtree,
                                                  pcie::Controller::probe_pcie_host,
                                                  ArmGIC::probe_devtree,
                                                  virtio::MMIOTransport::probe_devtree};

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
extern u8 __stack_start;
extern u8 __stack_top;

mem::PhysicalRegion get_kernel_region() {
    auto phys_ptr = *mem::kernel_virt_to_phys(&__kernel_start);
    auto size     = &__kernel_end - &__kernel_start;
    return {phys_ptr, static_cast<uSize>(size)};
}

mem::VirtualRegion get_kernel_stack() { return {{&__stack_start}, static_cast<uSize>(&__stack_top - &__stack_start)}; }

extern "C" {
uPtr g_current_embedded_table_phys;
}

bek::OutputStream* debug_stream  = nullptr;

using DBG = DebugScope<"Kern", true>;

extern "C" [[noreturn]] void kernel_boot(u64 dev_tree_address) {
    // 1. Setup Debug UART
    PL011 uart{qemu_pl011_address, qemu_clock_freq};
    debug_stream = &uart;
    uart.write("Hello Kernel World!\n"_sv);

    // 2. Set interrupt vector table (easier debugging if fault)
    do_set_vector_table();

    // 3. Initialise memory allocation (from statically allocated 1MB region)
    mem::initialise_kmalloc();

    // 4. Parse DeviceTree, and get a map of Physical RAM
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

    // 5. Using Physical map of space, initialise the memory manager.
    //  (a) Map all the physical memory, using embedded tables initially (and then PageAllocator itself!)
    mem::MemoryManager::initialise(
        memory_space, static_cast<u8*>(mem::kernel_phys_to_virt(mem::PhysicalPtr(g_current_embedded_table_phys))));

    // 6. Probe DeviceTree
    dev_tree::probe_nodes(dtb, bek::span{standard_probes.begin(), standard_probes.end()});

    mem::log_kmalloc_usage();

    // 7. Enable interrupts + timing, so middle-stage device initialisation and setup can occur.
    deferred::initialise();
    enable_interrupts();
    if (auto r = timing::initialise(); r != ESUCCESS) {
        DBG::dbgln("Failed to initialise timing: {}"_sv, r);
        PANIC("Critical Startup Failure");
    }

    // 8. Initialise and enter kernel task.
    auto r = ProcessManager::initialise_and_adopt(bek::string{"ktask"}, get_kernel_stack());

    if (r == ESUCCESS) {
        DBG::dbgln("Running in process, PID {}!"_sv, ProcessManager::the().current_process().pid());
    } else {
        DBG::dbgln("Failed to transition into process: {}"_sv, r);
    }

    // 9. Find block devices
    for (int tries = 0;; tries++) {
        auto mount_result = fs::FilesystemRegistry::try_mount_root();
        if (mount_result == ESUCCESS) break;
        if (tries == 5 || !(mount_result == ENODEV || mount_result == EINVAL)) {
            DBG::dbgln("Failed to mount root: {}"_sv, mount_result);
            PANIC("Could not successfully mount root.");
        }
        timing::spindelay_us(1'000'000);
    }

    auto root_r = fs::fullPathLookup({}, "/"_sv, nullptr);
    VERIFY(root_r.has_value());
    auto& root = root_r.value();

    auto init_exec_r = fs::fullPathLookup({}, "/init"_sv, nullptr);

    if (init_exec_r.has_error()) {
        DBG::dbgln("Could not find init executable: {}."_sv, init_exec_r.error());
        PANIC("init execute failed.");
    }
    auto& init_exec = init_exec_r.value();

    bek::vector<Process::LocalEntityHandle> init_handles{
        {bek::adopt_shared(new ProcessDebugSerial()), 0},
        {bek::adopt_shared(new NullHandle()), 0},
        {bek::adopt_shared(new ProcessDebugSerial()), 0},
    };

    auto proc_r = Process::spawn_user_process(bek::string{"init"}, init_exec, root, bek::move(init_handles));

    if (proc_r.has_error()) {
        DBG::dbgln("Could not spawn init process: {}"_sv, proc_r.error());
        PANIC("Could not spawn init process");
    }

    auto& proc = *proc_r.value();

    proc.set_state(ProcessState::Running);

    while (true) {
        DBG::dbgln("Noot"_sv);
        timing::spindelay_us(10'000'000);
    }
}