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

#ifndef BEKOS_ARM_GIC_H
#define BEKOS_ARM_GIC_H

#include "bek/buffer.h"
#include "bek/optional.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "library/function.h"
#include "peripherals/device_tree.h"
#include "peripherals/interrupt_controller.h"

class ArmGIC : public InterruptController {
public:
    static ArmGIC create(mem::DeviceArea distributor_base, mem::DeviceArea cpu_base);
    static dev_tree::DevStatus probe_devtree(dev_tree::Node& node, dev_tree::device_tree&, dev_tree::probe_ctx& ctx);

    Handle register_interrupt(bek::buffer selection_data) override;
    void register_handler(u32 interrupt_id, InterruptHandler&& handler) override;
    void enable_interrupt(u32 interrupt_id) override;
    void disable_interrupt(u32 interrupt_id) override;

    void handle_interrupt() override;

private:
    ArmGIC(mem::DeviceArea distributor_area, mem::DeviceArea cpu_base, u8 num_cpus, u32 max_ids);
    bool initialise();

    bek::vector<bek::optional<InterruptHandler>> m_handlers;

    u32 m_num_cpus;
    u32 m_num_ids;

    mem::DeviceArea m_distributor_base;
    mem::DeviceArea m_cpu_base;
};

#endif  // BEKOS_ARM_GIC_H
