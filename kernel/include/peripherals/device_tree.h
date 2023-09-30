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

#ifndef BEKOS_DEVICE_TREE_H
#define BEKOS_DEVICE_TREE_H

#include "device.h"
#include "kstring.h"
#include "library/buffer.h"
#include "library/format_core.h"
#include "library/hashtable.h"
#include "library/optional.h"
#include "library/own_ptr.h"
#include "library/span.h"
#include "library/string.h"
#include "library/types.h"
#include "library/vector.h"
#include "mm/addresses.h"

namespace dev_tree {

enum node_status { Okay, Disabled, Reserved, Fail, FailSSS };

struct reg_t {
    u64 address;
    u64 size;
};

struct range_t {
    u64 child_address;
    u64 parent_address;
    u64 size;
};

struct property {
    bek::str_view name;
    bek::buffer data;
};

struct reserved_region {
    u64 address;
    u64 size;
};

enum class DevStatus { Unprobed, Unrecognised, Waiting, Success, Failure };

struct Node {
    bek::str_view name;
    bek::vector<bek::own_ptr<Node>> children;
    Node* parent;
    bek::vector<bek::str_view> compatible;
    bek::vector<property> properties;
    u32 phandle;
    node_status status;

    bek::own_ptr<Device> attached_device;
    DevStatus nodeStatus;

    bek::optional<bek::buffer> get_property(bek::str_view name) const;
};

u64 read_from_buffer(bek::buffer b, u32& offset, u32 cells);

struct RangeArray {
    struct Iterator {
        bek::buffer remaining_range;
        u32 parent_cells;
        u32 child_cells;
        u32 size_cells;

        range_t operator*() const {
            u32 offset = 0;
            range_t range{};
            range.child_address  = read_from_buffer(remaining_range, offset, child_cells);
            range.parent_address = read_from_buffer(remaining_range, offset, parent_cells);
            range.size           = read_from_buffer(remaining_range, offset, size_cells);
            return range;
        }

        Iterator& operator++() {
            auto offset = (parent_cells + child_cells + size_cells) * 4;
            ASSERT(remaining_range.size() >= offset);
            remaining_range = remaining_range.subdivide(offset, remaining_range.size() - offset);
        }

        constexpr bool operator==(const Iterator&) const = default;
    };

    bek::buffer whole_range;
    u32 parent_cells;
    u32 child_cells;
    u32 size_cells;

    [[nodiscard]] constexpr Iterator begin() const {
        return {whole_range, parent_cells, child_cells, size_cells};
    }

    [[nodiscard]] constexpr Iterator end() const {
        return {whole_range.subdivide(whole_range.size(), 0), parent_cells, child_cells,
                size_cells};
    }

    [[nodiscard]] constexpr u32 size() const {
        return whole_range.size() / (4 * (parent_cells + child_cells + size_cells));
    }

    [[nodiscard]] constexpr bool can_dereference_iterator() const {
        return parent_cells <= 2 && child_cells <= 2 && size_cells <= 2;
    }
};

bek::vector<bek::str_view> parse_stringlist(bek::buffer data);

bek::optional<u32> get_property_u32(const Node& t_node, bek::str_view name);

bek::vector<reg_t> get_std_regs(const Node& t_node);

bek::vector<mem::PhysicalRegion> get_regions_from_reg(const Node& node);

bek::optional<RangeArray> get_ranges(const Node& node, bool dma_ranges = false);

bek::optional<mem::PhysicalPtr> map_address_to_phys(uPtr address, const Node& node);

bek::optional<mem::PhysicalRegion> map_region_to_root(const Node& node, mem::PhysicalRegion region);
bek::vector<range_t> get_dma_to_phys_ranges(const Node& node);

struct device_tree {
    bek::vector<reserved_region> reserved_regions{};
    bek::own_ptr<Node> root_node{};
    bek::hashtable<u32, Node*> phandles{};
    bek::vector<Node*> waiting{};
};

bek::pair<Node*, DevStatus> get_node_by_phandle(const device_tree& tree, u32 phandle);
bek::optional<u32> get_inheritable_property_u32(const Node& node, bek::str_view name);
bek::vector<mem::PhysicalRegion> get_memory_regions(const device_tree& tree);
bek::vector<mem::PhysicalRegion> get_reserved_regions(const device_tree& tree);

device_tree read_dtb(bek::buffer dtb);

using ProbeFn = DevStatus(Node&, device_tree&);

void bek_basic_format(bek::OutputStream& out, const reg_t& reg);
void bek_basic_format(bek::OutputStream& out, const range_t& reg);
}  // namespace dev_tree
#endif  // BEKOS_DEVICE_TREE_H
