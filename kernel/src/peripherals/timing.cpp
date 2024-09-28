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

#include "interrupts/int_ctrl.h"
#include "library/debug.h"
#include "library/intrusive_list.h"
#include "peripherals/timer.h"

using DBG = DebugScope<"Timing", DebugLevel::WARN>;

inline constexpr u64 operation_ns_estimate = 10;
inline constexpr u64 nanoseconds_per_s = 1'000'000'000;

class TimingManager {
public:
    explicit TimingManager(bek::shared_ptr<TimerDevice>&& device)
        : m_device{bek::move(device)},
          m_operation_ticks_estimate{(operation_ns_estimate * m_device->get_frequency()) / nanoseconds_per_s} {};

    ErrorCode schedule_callback(bek::function<TimerDevice::CallbackAction(u64)>&& action, long period_ns) {
        VERIFY(period_ns >= 0);
        auto period = bek::max((period_ns * m_device->get_frequency()) / nanoseconds_per_s, m_operation_ticks_estimate);
        auto* node = new TimingNode{bek::move(action), {}, period, 0};
        if (!node) return ENOMEM;
        queue_node(*node);
        return ESUCCESS;
    }

    uSize nanoseconds_since_start() { return m_device->get_ticks() * nanoseconds_per_s / m_device->get_frequency(); }

private:
    struct TimingNode {
        bek::function<TimerDevice::CallbackAction(u64)> action;
        bek::IntrusiveListNode<TimingNode> node;
        /// The number of ticks between triggering.
        uSize period;
        /// The value of ticks at next trigger.
        uSize next_trigger;
        using List = bek::IntrusiveList<TimingNode, &TimingNode::node>;
    };

    bek::shared_ptr<TimerDevice> m_device;
    TimingNode::List m_pending_timers;
    u64 m_operation_ticks_estimate;

    void queue_node(TimingNode& node) {
        if (node.period <= m_operation_ticks_estimate) {
            DBG::dbgln("Warn: Short tick period: {}"_sv, node.period);
        }
        // This may get old - by design?
        auto current_ticks = m_device->get_ticks();
        node.next_trigger = current_ticks + node.period;
        // Need to loop through.
        bool inserted = false;
        {
            InterruptDisabler disabler;
            for (auto& other_node : m_pending_timers) {
                // FIXME: Assuming monotonically _increasing_ counter.
                if (node.next_trigger < other_node.next_trigger) {
                    // We insert!
                    m_pending_timers.insert_before(other_node, node);  // Ruins iterator.
                    inserted = true;
                    break;
                }
            }
            if (!inserted) m_pending_timers.append(node);
        }
        if (m_pending_timers.front_ptr() == &node) {
            set_next_tick(node.period);
        }
    }

    void set_next_tick(u64 tick) {
        m_device->schedule_callback([this]() { return on_trigger(); }, static_cast<long>(tick));
    }

    TimerDevice::CallbackAction on_trigger() {
        auto current_ticks = m_device->get_ticks();
        auto* front_action = m_pending_timers.front_ptr();
        while (front_action && front_action->next_trigger < current_ticks) {
            front_action->node.remove();
            if (auto result = front_action->action(current_ticks); result.is_reschedule()) {
                // Period is in NANOSECONDS
                front_action->period = (result.period * m_device->get_frequency()) / nanoseconds_per_s;
                queue_node(*front_action);
            } else {
                delete front_action;
                front_action = nullptr;
            }
            current_ticks = m_device->get_ticks();
            front_action = m_pending_timers.front_ptr();
        }
        if (!front_action) {
            return TimerDevice::CallbackAction::Cancel;
        } else {
            return TimerDevice::CallbackAction::Reschedule(
                bek::max<long>(static_cast<long>(front_action->next_trigger - current_ticks),
                               static_cast<long>(m_operation_ticks_estimate)));
        }
    }
};

TimingManager* g_timing_manager = nullptr;

ErrorCode timing::initialise() {
    // TODO: What happens if we have two timers?
    bek::shared_ptr<TimerDevice> dev = nullptr;
    DeviceRegistry::the().for_each_device([&dev](bek::str_view name, bek::shared_ptr<Device>& candidate_dev) {
        if (candidate_dev->kind() == Device::Kind::Timer) {
            dev = Device::as<TimerDevice>(candidate_dev.get());
            return IterationDecision::Break;
        } else {
            return IterationDecision::Continue;
        }
    });

    if (dev == nullptr) {
        DBG::dbgln("Err: No timer device found!"_sv);
        return EFAIL;
    }

    g_timing_manager = new TimingManager(bek::move(dev));
    if (!g_timing_manager) return ENOMEM;
    return ESUCCESS;
}
ErrorCode timing::schedule_callback(bek::function<TimerDevice::CallbackAction(u64)> action, long nanoseconds) {
    VERIFY(g_timing_manager);
    return g_timing_manager->schedule_callback(bek::move(action), nanoseconds);
}
void timing::spindelay_us(uSize microseconds) {
    VERIFY(g_timing_manager);
    volatile bool completed = false;
    auto r = g_timing_manager->schedule_callback(
        [&completed](u64) {
            completed = true;
            return TimerDevice::CallbackAction::Cancel;
        },
        microseconds * 1000ul);
    while (r == ESUCCESS && !completed) {
        asm volatile("nop");
    }
}
uSize timing::nanoseconds_since_start() { return g_timing_manager->nanoseconds_since_start(); }
