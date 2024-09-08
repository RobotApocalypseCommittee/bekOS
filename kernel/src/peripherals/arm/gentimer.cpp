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

#include "peripherals/arm/gentimer.h"

u64 ArmGenericTimer::get_frequency() {
    u64 frequency;
    asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(frequency));
    return frequency;
}
u64 ArmGenericTimer::get_ticks() {
    u64 ticks;
    asm volatile("mrs %0, CNTPCT_EL0" : "=r"(ticks));
    return ticks;
}
bool ArmGenericTimer::schedule_callback(bek::function<CallbackAction()> function, long ticks) {
    m_callback = static_cast<bek::function<CallbackAction(void), false, true>>(bek::move(function));
    auto ticks_trigger = ticks + get_ticks();
    asm volatile("msr CNTP_CVAL_EL0, %0" : : "r"(ticks_trigger));
    u64 control_flags = 0b001;  // Un-Mask Interrupts, Enable
    asm volatile("msr CNTP_CTL_EL0, %0" : : "r"(control_flags));
    return true;
}
bek::shared_ptr<TimerDevice> ArmGenericTimer::probe_timer(InterruptController& global_intc) {
    u32 int_detail[]{bek::swapBytes(1u), bek::swapBytes(14u), bek::swapBytes(0xf08u)};
    auto handle = global_intc.register_interrupt(bek::buffer{(char*)int_detail, sizeof int_detail});
    if (!handle) return nullptr;
    auto timer = bek::make_shared<ArmGenericTimer>();
    handle.register_handler(InterruptHandler{[timer]() { timer->on_trigger(); }});
    handle.enable();
    return timer;
}
void ArmGenericTimer::on_trigger() {
    if (m_callback) {
        auto res = m_callback();
        if (res.is_reschedule()) {
            auto ticks_trigger = res.period + get_ticks();
            asm volatile("msr CNTP_CVAL_EL0, %0" : : "r"(ticks_trigger));
        } else {
            u64 control_flags = 0b110;  // Mask Interrupts, Disable
            asm volatile("msr CNTP_CTL_EL0, %0" : : "r"(control_flags));
        }
    } else {
        u64 control_flags = 0b110;  // Mask Interrupts, Disable
        asm volatile("msr CNTP_CTL_EL0, %0" : : "r"(control_flags));
    }
}
ArmGenericTimer::ArmGenericTimer() {
    u64 control_flags = 0b110;  // Mask Interrupts, Disable
    asm volatile("msr CNTP_CTL_EL0, %0" : : "r"(control_flags));
}
