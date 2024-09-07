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

#ifndef BEKOS_CLOCK_H
#define BEKOS_CLOCK_H

#include "device.h"
#include "device_tree.h"
class Clock : public Device {
public:
    using DeviceType                 = Clock;
    static constexpr Kind DeviceKind = Device::Kind::Clock;

    virtual u64 get_frequency() = 0;

    Kind kind() const override { return Kind::Clock; }
    bek::str_view preferred_name_prefix() const override { return "generic.clock"_sv; }
};

class FixedClock : public Clock {
public:
    explicit FixedClock(u64 frequency) : frequency{frequency} {}

    static dev_tree::DevStatus probe_devtree(dev_tree::Node& node, dev_tree::device_tree& tree, dev_tree::probe_ctx&);

    u64 get_frequency() override { return frequency; }

private:
    u64 frequency;
};

inline dev_tree::DevStatus FixedClock::probe_devtree(dev_tree::Node& node, dev_tree::device_tree&,
                                                     dev_tree::probe_ctx&) {
    if (!(node.compatible.size() && node.compatible[0] == "fixed-clock"_sv))
        return dev_tree::DevStatus::Unrecognised;
    auto prop_opt = node.get_property("clock-frequency"_sv);
    if (!prop_opt) return dev_tree::DevStatus::Failure;
    auto buf = *prop_opt;
    if (buf.size() == 4) {
        node.attached_device = bek::make_own<FixedClock>(buf.get_at_be<u32>(0));
        return dev_tree::DevStatus::Success;
    } else if (buf.size() == 8) {
        node.attached_device = bek::make_own<FixedClock>(buf.get_at_be<u64>(0));
        return dev_tree::DevStatus::Success;
    } else {
        return dev_tree::DevStatus::Failure;
    }
}

static_assert(bek::implictly_convertible_to<FixedClock*, Device*>);

#endif  // BEKOS_CLOCK_H
