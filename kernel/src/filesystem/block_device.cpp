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

#include "filesystem/block_device.h"

#include "filesystem/partition.h"
#include "library/debug.h"
#include "library/format.h"

using DBG = DebugScope<"BlkDev", true>;

namespace blk {
static BlockDeviceRegistry* g_registry = nullptr;

BlockDeviceRegistry& BlockDeviceRegistry::the() {
    if (!g_registry) {
        g_registry = new BlockDeviceRegistry();
    }
    return *g_registry;
}
bek::pair<bek::string, u32> BlockDeviceRegistry::allocate_identifiers(bek::str_view prefix) {
    bek::string prefix_str{prefix};
    auto id = global_next_id++;

    u32 name_suffix = 0;
    auto x          = m_next_ids.find(prefix_str);
    if (x) {
        name_suffix = (*x)++;
    } else {
        m_next_ids.insert({prefix_str, 1});
    }

    return {bek::format("{}{}"_sv, prefix, name_suffix), id};
}
void BlockDeviceRegistry::register_raw_device(bek::own_ptr<BlockDevice> device) {
    m_raw_devices.push_back(bek::move(device));
    probe_block_device(*m_raw_devices.back(), bek::function<void(bek::vector<PartitionInfo>)>(
                                                  [](bek::vector<PartitionInfo> x) {
                                                      DBG::dbgln("{} partitions:"_sv, x.size());
                                                      for (auto info : x) {
                                                          DBG::dbgln("    Partition: {}"_sv, info);
                                                      }
                                                  }));
}

}  // namespace blk
