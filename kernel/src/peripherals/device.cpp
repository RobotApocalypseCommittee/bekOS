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

#include "peripherals/device.h"

#include "bek/format.h"
#include "library/debug.h"

using DBG = DebugScope<"DevMgmt", DebugLevel::INFO>;

static DeviceRegistry* g_device_registry = nullptr;

DeviceRegistry& DeviceRegistry::the() {
    if (!g_device_registry) {
        g_device_registry = new DeviceRegistry();
    }
    return *g_device_registry;
}

bek::str_view DeviceRegistry::register_device(bek::str_view name_prefix, bek::shared_ptr<Device> device) {
    int i = 0;
    auto generated_name = bek::format("{}{}"_sv, name_prefix, i);
    while (m_devices.find(generated_name)) {
        i++;
        generated_name = bek::format("{}{}"_sv, name_prefix, i);
    }
    auto it = m_devices.insert({bek::move(generated_name), bek::move(device)});
    VERIFY(it.second);
    DBG::infoln("Registered Device: {}"_sv, it.first->first.view());
    return it.first->first.view();
}

bek::shared_ptr<Device> DeviceRegistry::get(bek::str_view name) const {
    bek::string n{name};
    if (auto it = m_devices.find(n); it) {
        return *it;
    } else {
        return nullptr;
    }
}
bek::shared_ptr<DeviceHandle> DeviceRegistry::open(bek::str_view name) const {
    if (auto dev = get(name); dev) {
        return bek::adopt_shared(new DeviceHandle{bek::move(dev)});
    } else {
        return nullptr;
    }
}
