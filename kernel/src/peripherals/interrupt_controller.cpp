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

#include "peripherals/interrupt_controller.h"

#include "bek/assertions.h"
#include "bek/utility.h"
#include "peripherals/peripherals.h"

#define INTCTL_BASE (PERIPHERAL_BASE + 0xB000)

#define INTCTL_BASIC_PENDING (0x200)
#define INTCTL_PENDING_1 (0x204)
#define INTCTL_PENDING_2 (0x208)
#define INTCTL_FIQCTL (0x20C)
#define INTCTL_ENIRQ_1 (0x210)
#define INTCTL_ENIRQ_2 (0x214)
#define INTCTL_EN_BASIC (0x218)
#define INTCTL_DISIRQ_1 (0x21C)
#define INTCTL_DISIRQ_2 (0x220)
#define INTCTL_DIS_BASIC (0x224)

void LegacyInterruptController::enable(LegacyInterruptController::interrupt_type t) {
    unsigned int x = t;
    if (x >= 64) {
        // 'BASIC' interrupt
        m_area.write<u32>(INTCTL_EN_BASIC, 1u << (x - 64));
    } else if (x < 32) {
        m_area.write<u32>(INTCTL_ENIRQ_1, 1u << x);
    } else {
        m_area.write<u32>(INTCTL_ENIRQ_2, 1u << (x - 32));
    }
}

void LegacyInterruptController::disable(LegacyInterruptController::interrupt_type t) {
    unsigned int x = t;
    if (x >= 64) {
        // 'BASIC' interrupt
        m_area.write<u32>(INTCTL_DIS_BASIC, 1u << (x - 64));
    } else if (x < 32) {
        m_area.write<u32>(INTCTL_DISIRQ_1, 1u << x);
    } else {
        m_area.write<u32>(INTCTL_DISIRQ_2, 1u << (x - 32));
    }
}

bool LegacyInterruptController::handle() {
    interrupt_type int_type = interrupt_type::NONE;
    unsigned int basic_ints = m_area.read<u32>(INTCTL_BASIC_PENDING);
    int basic_bit_set = -1;
    for (int i = 0; i < 32; i++) {
        if (basic_ints & (1u << i)) {
            basic_bit_set = i;
            break;
        }
    }
    if (basic_bit_set == -1) { return false; }
    if (basic_bit_set == 8) {
        unsigned int ints1 = m_area.read<u32>(INTCTL_PENDING_1);
        for (unsigned int i = 0; i < 32; i++) {
            if (ints1 & (1u << i)) {
                // This interrupt
                int_type = static_cast<interrupt_type>(i);
                break;
            }
        }
    } else if (basic_bit_set == 9) {
        // Pending Register 2
        unsigned int ints2 = m_area.read<u32>(INTCTL_PENDING_2);
        for (unsigned int i = 0; i < 32; i++) {
            if (ints2 & (1u << i)) {
                // This interrupt
                int_type = static_cast<interrupt_type>(i + 32);
                break;
            }
        }
    } else if (basic_bit_set <= 7) {
        // Maps directly to basic interrupts
        int_type = static_cast<interrupt_type>(basic_bit_set + 64);
    } else {
        // GPU interrupts; dont know yet
        return false;
    }
    // Dispatch
    if (int_type < 96) {
        // Within range...
        if (handlers[int_type]) {
            (*handlers[int_type])();
            return true;
        } else {
            // No registered handler
            return false;
        }
    } else {
        return false;
    }
}

void LegacyInterruptController::register_handler(
    LegacyInterruptController::interrupt_type interruptType, InterruptHandler&& handler) {
    ASSERT(interruptType < 96);
    ASSERT(!handlers[interruptType]);
    handlers[interruptType] = bek::move(handler);
}
Device::Kind InterruptController::kind() const { return Device::Kind::InterruptController; }
bool InterruptController::is_userspace_accessible() const { return false; }
