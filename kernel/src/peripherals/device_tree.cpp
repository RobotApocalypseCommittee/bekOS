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

#include "peripherals/device_tree.h"

#include "bek/format.h"
#include "bek/span.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "library/debug.h"

using namespace dev_tree;
#define EX_BYTE_32(x, n) static_cast<u32>(reinterpret_cast<u8*>(&x)[n])
#define EX_BYTE_64(x, n) static_cast<u64>(reinterpret_cast<u8*>(&x)[n])

using DBG = DebugScope<"DevTree", true>;

constexpr const unsigned int DTB_MAGIC = 0xd00dfeed;

constexpr inline bek::str_view ADDRESS_CELLS_TAG = "#address-cells"_sv;
constexpr inline bek::str_view SIZE_CELLS_TAG    = "#size-cells"_sv;

u32 big_to_cpu(u32 x) {
    return EX_BYTE_32(x, 0) << 24 | EX_BYTE_32(x, 1) << 16 | EX_BYTE_32(x, 2) << 8 |
           EX_BYTE_32(x, 3);
}

u64 big_to_cpu(u64 x) {
    return EX_BYTE_64(x, 0) << 56 | EX_BYTE_64(x, 1) << 48 | EX_BYTE_64(x, 2) << 40 |
           EX_BYTE_64(x, 3) << 32 | EX_BYTE_64(x, 4) << 24 | EX_BYTE_64(x, 5) << 16 |
           EX_BYTE_64(x, 6) << 8 | EX_BYTE_64(x, 7);
}

struct fdt_header {
    u32 magic;
    u32 totalsize;
    u32 off_dt_struct;
    u32 off_dt_strings;
    u32 off_mem_rsvmap;
    u32 version;
    u32 last_comp_version;
    u32 boot_cpuid_phys;
    u32 size_dt_strings;
    u32 size_dt_struct;
};

enum dtb_struct_tag : u32 {
    BEGIN_NODE = 1,
    END_NODE   = 2,
    BEGIN_PROP = 3,
    NOP        = 4,
    END        = 9,
};

struct dtb_string_getter {
    explicit dtb_string_getter(const char* start, uSize size) : strings_data(start, size) {}

    [[nodiscard]] bek::str_view get_string(u32 offset) const {
        auto begin = strings_data.data() + offset;
        auto len = bek::strlen(begin);
        return {begin, len};
    }

private:
    bek::span<const char> strings_data;
};

bek::vector<bek::str_view> dtb_read_stringlist(bek::buffer buffer) {
    const char* ptr = buffer.data();
    const char* end = buffer.end();
    auto res        = bek::vector<bek::str_view>{};
    while (ptr < end) {
        auto len = bek::strlen(ptr);
        res.push_back({ptr, len});
        ptr += len + 1;
    }
    return res;
}

struct dtb_struct_pointer {
    explicit dtb_struct_pointer(const char* start, uSize size) : buf{start}, end{start + size} {}
    u32 read_u32() {
        ASSERT(buf + 4 <= end);
        auto res = big_to_cpu(*reinterpret_cast<const u32*>(buf));
        buf += 4;
        return res;
    }

    dtb_struct_tag read_tag() {
        ASSERT((reinterpret_cast<uPtr>(buf) & 0b11) == 0);
        return static_cast<dtb_struct_tag>(read_u32());
    }

    dtb_struct_tag peek_tag() {
        ASSERT((reinterpret_cast<uPtr>(buf) & 0b11) == 0);
        ASSERT(buf + 4 <= end);
        return static_cast<dtb_struct_tag>(big_to_cpu(*reinterpret_cast<const u32*>(buf)));
    }

    const char* data() const { return buf; }

    void read_nops() {
        while (peek_tag() == NOP) {
            buf += 4;
        }
    }

    bek::str_view read_null_string() {
        auto len = bek::strlen(reinterpret_cast<const char*>(buf));
        auto str = bek::str_view{reinterpret_cast<const char*>(buf), len};
        buf += len + 1;
        return str;
    }

    void read_padding() {
        while ((reinterpret_cast<uPtr>(buf) & 0b11) != 0) {
            // ASSERT(*buf == 0);
            buf++;
        }
    }

    bek::buffer read_buffer(uSize size) {
        ASSERT(buf + size <= end);
        auto res = bek::buffer{buf, size};
        buf += size;
        return res;
    }

private:
    const char* buf;
    const char* end;
};
/// Consumes start Node tag
bek::own_ptr<Node> parse_node(dtb_struct_pointer& cursor, dtb_string_getter strings, Node* parent,
                              bek::hashtable<u32, Node*>& phandles) {
    cursor.read_nops();
    ASSERT(cursor.read_tag() == BEGIN_NODE);
    auto name     = cursor.read_null_string();
    auto new_node = bek::make_own<Node>(
        Node{name, {}, parent, {}, {}, 1000, Okay, nullptr, dev_tree::DevStatus::Unprobed});

    cursor.read_padding();
    while (cursor.peek_tag() == BEGIN_PROP) {
        cursor.read_tag();
        auto len       = cursor.read_u32();
        auto name_off  = cursor.read_u32();
        auto prop_data = cursor.read_buffer(len);
        cursor.read_padding();
        auto prop_name = strings.get_string(name_off);

        new_node->properties.push_back(property{prop_name, prop_data});

        // Special Cases
        if (prop_name == "compatible"_sv) {
            new_node->compatible = dtb_read_stringlist(prop_data);
        } else if (prop_name == "phandle"_sv) {
            new_node->phandle = prop_data.get_at_be<u32>(0);
            phandles.insert({new_node->phandle, new_node.get()});
        }
    }
    while (cursor.peek_tag() == BEGIN_NODE) {
        new_node->children.push_back(parse_node(cursor, strings, new_node.get(), phandles));
    }
    cursor.read_nops();
    ASSERT(cursor.read_tag() == END_NODE);
    return new_node;
}

device_tree dev_tree::read_dtb(bek::buffer dtb) {
    fdt_header header;

    // Copy data and adjust endianness
    for (int i = 0; i < sizeof(fdt_header) / sizeof(u32); i++) {
        reinterpret_cast<u32*>(&header)[i] =
            big_to_cpu(reinterpret_cast<const u32*>(dtb.data())[i]);
    }

    // Check Magic Value
    if (header.magic != DTB_MAGIC) {
        DBG::dbgln("Disaster."_sv);
        return {};
    }
    DBG::dbgln("Loading with version {}."_sv, header.version);

    // Read Reserved Blocks
    bek::vector<reserved_region> reserved_regions{};
    auto reserved_blob =
        reinterpret_cast<const reserved_region*>(dtb.data() + header.off_mem_rsvmap);
    while (!(reserved_blob->address == 0 && reserved_blob->size == 0)) {
        reserved_regions.push_back(
            {big_to_cpu(reserved_blob->address), big_to_cpu(reserved_blob->size)});
        reserved_blob++;
    }

    // Read Structures
    auto struct_buf = dtb.subdivide(header.off_dt_struct, header.size_dt_struct);
    dtb_struct_pointer cursor{struct_buf.data(), struct_buf.size()};

    auto strings_buf = dtb.subdivide(header.off_dt_strings, header.size_dt_strings);
    dtb_string_getter strings{strings_buf.data(), strings_buf.size()};

    bek::hashtable<u32, Node*> phandles{};

    // First, read until start of root Node.
    auto root_node = parse_node(cursor, strings, nullptr, phandles);

    return {bek::move(reserved_regions), bek::move(root_node), bek::move(phandles)};
}

bek::optional<u32> dev_tree::get_property_u32(const dev_tree::Node& t_node, bek::str_view name) {
    auto prop = t_node.get_property(name);
    if (prop.is_valid()) {
        ASSERT(prop->size() == 4);
        return {prop->get_at_be<u32>(0)};
    } else {
        return {};
    }
}

bek::vector<reg_t> dev_tree::get_std_regs(const Node& t_node) {
    auto buf_opt = t_node.get_property("reg"_sv);
    if (!buf_opt) {
        return {};
    }
    auto buf = *buf_opt;

    u32 addr_cells = 2;
    u32 size_cells = 1;
    if (t_node.parent) {
        addr_cells = get_property_u32(*t_node.parent, ADDRESS_CELLS_TAG).value_or(2);
        size_cells = get_property_u32(*t_node.parent, SIZE_CELLS_TAG).value_or(1);
    }

    // We only handle max u64 addresses and sizes - otherwise nonsensical!
    ASSERT(addr_cells == 1 || addr_cells == 2);
    ASSERT(size_cells == 1 || size_cells == 2);

    int n = buf.size() / (4 * (addr_cells + size_cells));
    ASSERT(buf.size() == n * 4 * (addr_cells + size_cells));

    u32 offset = 0;

    bek::vector<reg_t> result{};
    result.reserve(n);

    while (n > 0) {
        // Extract addr, then size.
        reg_t reg{};
        if (addr_cells == 1) {
            reg.address = buf.get_at_be<u32>(offset);
            offset += 4;
        } else {
            reg.address = buf.get_at_be<u64>(offset);
            offset += 8;
        }

        if (size_cells == 1) {
            reg.size = buf.get_at_be<u32>(offset);
            offset += 4;
        } else {
            reg.size = buf.get_at_be<u64>(offset);
            offset += 8;
        }
        result.push_back(reg);
        n--;
    }
    return result;
}

void dev_tree::bek_basic_format(bek::OutputStream& out, const range_t& reg) {
    bek::format_to(out, "{} -> {} ({})"_sv, reg.parent_address, reg.child_address, reg.size);
}
void dev_tree::bek_basic_format(bek::OutputStream& out, const reg_t& reg) {
    bek::format_to(out, "{} ({})"_sv, reg.address, reg.size);
}

u64 dev_tree::read_from_buffer(bek::buffer b, u32& offset, u32 cells) {
    ASSERT(cells <= 2);
    u64 ret_val;
    switch (cells) {
        case 0:
            return 0;
        case 1:
            ret_val = b.get_at_be<u32>(offset);
            offset += 4;
            return ret_val;
        case 2:
            ret_val = b.get_at_be<u64>(offset);
            offset += 8;
            return ret_val;
        default:
            ASSERT_UNREACHABLE();
    }
}
bek::pair<Node*, DevStatus> dev_tree::get_node_by_phandle(const device_tree& tree, u32 phandle) {
    auto it = tree.phandles.find(phandle);
    if (!it) {
        DBG::dbgln("Could not find node with phandle {}."_sv, phandle);
        return {nullptr, DevStatus::Failure};
    }
    auto& node = **it;
    switch (node.nodeStatus) {
        case DevStatus::Unprobed:
        case DevStatus::Waiting:
            DBG::dbgln("Waiting for node {}."_sv, node.name);
            return {nullptr, DevStatus::Waiting};
        case DevStatus::Unrecognised:
        case DevStatus::Failure:
            DBG::dbgln("Relying on bad device {}."_sv, node.name);
            return {nullptr, DevStatus::Failure};
        case DevStatus::Success:
            return {&node, DevStatus::Success};
        default:
            ASSERT_UNREACHABLE();
    }
}
bek::optional<u32> dev_tree::get_inheritable_property_u32(const dev_tree::Node& node,
                                                          bek::str_view name) {
    auto* p_node = &node;
    bek::optional<u32> value{};
    while (p_node && !value) {
        value  = get_property_u32(*p_node, name);
        p_node = p_node->parent;
    }
    return value;
}

bek::optional<RangeArray> dev_tree::get_ranges(const Node& node, bool dma_ranges) {
    auto buf = node.get_property(dma_ranges ? "dma-ranges"_sv : "ranges"_sv);
    if (!buf) {
        return {};
    }

    u32 this_addr_cells   = get_property_u32(node, ADDRESS_CELLS_TAG).value_or(2);
    u32 size_cells        = get_property_u32(node, SIZE_CELLS_TAG).value_or(1);
    u32 parent_addr_cells = 2;
    if (node.parent) {
        parent_addr_cells = get_property_u32(*node.parent, ADDRESS_CELLS_TAG).value_or(2);
    }

    ASSERT(buf->size() % (parent_addr_cells + this_addr_cells + size_cells) == 0);
    return RangeArray{*buf, parent_addr_cells, this_addr_cells, size_cells};
}
bek::optional<mem::PhysicalRegion> dev_tree::map_region_to_root(const Node& node,
                                                                mem::PhysicalRegion region) {
    auto* parent = node.parent;
    while (parent) {
        if (!parent->parent) {
            // Node is root node - we are done.
            break;
        }
        auto range_arr = get_ranges(node);
        if (!range_arr || !range_arr->can_dereference_iterator()) {
            DBG::dbgln("Map Address: {} has absent or malformed ranges property."_sv, parent->name);
            return {};
        }
        if (range_arr->size() == 0) {
            // Identity Mapping
            parent = parent->parent;
            continue;
        }

        bool found_mapping = false;
        for (auto range : *range_arr) {
            if (mem::PhysicalRegion{{range.child_address}, range.size}.contains(region)) {
                region = mem::PhysicalRegion{
                    {range.parent_address + (region.start.ptr - range.child_address)}, region.size};
                parent        = parent->parent;
                found_mapping = true;
                break;
            }
        }
        if (!found_mapping) {
            // Not part of any range.
            DBG::dbgln("Map Region: {:Xl} ({:X}) not contained in any range in {}"_sv,
                       region.start.ptr, region.size, parent->name);
            return {};
        }
    }
    return region;
}

constexpr inline range_t IDENTITY_MAPPING = {0, 0, ~static_cast<u64>(0)};

inline range_t apply_range(range_t lower, range_t upper) {
    // Get intersection of lower's parent space and upper's child space.
    auto mid_start = bek::max(lower.parent_address, upper.child_address);
    auto mid_end   = bek::min(lower.parent_address + lower.size, upper.child_address + upper.size);
    if (mid_end <= mid_start) {
        // No intersection
        return {0, 0, 0};
    }
    auto size         = mid_end - mid_start;
    auto lower_offset = mid_start - lower.parent_address;
    auto upper_offset = mid_start - upper.child_address;
    return {lower.child_address + lower_offset, upper.parent_address + upper_offset, size};
}

bek::vector<range_t> dev_tree::get_dma_to_phys_ranges(const Node& node) {
    // Get a mapping of children's dma addresses space -> (global) physical addresses.
    // We use current node's dma-ranges. If there isn't one, we assume an identity mapping.
    // TODO: No idea if this is correct. - probably isn't.

    if (node.parent == nullptr) {
        // Is root node. Cannot have dma-ranges?
        return {
            IDENTITY_MAPPING,
        };
    }
    auto range_arr = get_ranges(node, true);
    if (range_arr && !range_arr->can_dereference_iterator()) {
        // Unable to generate mappings at this moment.
        return {};
    }

    bek::vector<range_t> current_ranges{};
    // Assume identity mapping if dma-ranges is missing or empty.
    if (!range_arr || range_arr->size() == 0) {
        // Assume identity mapping
        // Not dodgy at all. - also it excludes the last byte :O
        current_ranges.push_back(IDENTITY_MAPPING);
    } else {
        for (auto range : *range_arr) {
            current_ranges.push_back(bek::move(range));
        }
    }

    // We have our ranges, now we need to map them through parent's 'ranges'. Cue code reuse.
    auto* parent = node.parent;
    while (parent) {
        if (!parent->parent) {
            // Node is root node - identity mappings always.
            break;
        }

        auto next_range_arr = get_ranges(*parent, false);
        // Q: if a device is DMA-capable, can it be not-memory-mapped? (e.g. SPI outwards, DMA in).
        if (!next_range_arr || !next_range_arr->can_dereference_iterator()) {
            DBG::dbgln("Map Address: {} has absent or malformed ranges property."_sv, parent->name);
            return {};
        }

        if (next_range_arr->size() == 0) {
            // Identity mapping
            parent = parent->parent;
            continue;
        }
        bek::vector<range_t> new_ranges;
        new_ranges.reserve(current_ranges.size());
        for (auto& range : current_ranges) {
            for (auto next_range : *next_range_arr) {
                // For each existing range, we try to apply each next_range to it.
                // The first one simply modifies the range in-place. If there a next one, it adds.
                auto new_mapping = apply_range(range, next_range);
                if (new_mapping.size) {
                    new_ranges.push_back(new_mapping);
                }
            }
        }
        current_ranges = bek::move(new_ranges);
        if (current_ranges.size() == 0) {
            return current_ranges;
        }
        parent = parent->parent;
    }
    return current_ranges;
}

bek::optional<mem::PhysicalPtr> dev_tree::map_address_to_phys(uPtr address, const Node& node) {
    auto o = dev_tree::map_region_to_root(node, mem::PhysicalRegion{{address}, 0});
    if (o) {
        return o->start;
    } else {
        return {};
    }
}
bek::vector<mem::PhysicalRegion> dev_tree::get_regions_from_reg(const Node& node) {
    auto buf = node.get_property("reg"_sv);
    if (!buf) {
        return {};
    }

    VERIFY(node.parent);
    u32 addr_cells = get_property_u32(*node.parent, ADDRESS_CELLS_TAG).value_or(2);
    u32 size_cells = get_property_u32(*node.parent, SIZE_CELLS_TAG).value_or(1);
    VERIFY(buf->size() % ((addr_cells + size_cells) * 4) == 0);

    RangeArray regs{*buf, addr_cells, 0, size_cells};
    VERIFY(regs.can_dereference_iterator());
    bek::vector<mem::PhysicalRegion> regions;
    regions.reserve(regs.size());
    for (auto reg : regs) {
        if (auto reg_mapped = map_region_to_root(node, {{
                                                            reg.parent_address,
                                                        },
                                                        reg.size})) {
            regions.push_back(*reg_mapped);
        } else {
            DBG::dbgln("Could not map register {:Xl} of device {}"_sv, reg.parent_address,
                       node.name);
        }
    }
    return regions;
}
bek::vector<mem::PhysicalRegion> dev_tree::get_memory_regions(const device_tree& tree) {
    bek::vector<mem::PhysicalRegion> regions;
    if (tree.root_node) {
        for (auto& node : tree.root_node->children) {
            if (node->name == "memory"_sv ||
                (node->name.size() >= 7 && node->name.substr(0, 7) == "memory@"_sv)) {
                // TODO: Can we make vector better / foreach lambda / iterator.
                auto node_regions = get_regions_from_reg(*node);
                for (auto& node_region : node_regions) {
                    regions.push_back(node_region);
                }
            }
        }
    }
    return regions;
}
bek::vector<mem::PhysicalRegion> dev_tree::get_reserved_regions(const device_tree& tree) {
    bek::vector<mem::PhysicalRegion> regions;
    for (auto region : tree.reserved_regions) {
        regions.push_back({{region.address}, region.size});
    }
    if (tree.root_node) {
        for (auto& node : tree.root_node->children) {
            if (node->name == "reserved-memory"_sv) {
                for (auto& reserved_node : node->children) {
                    auto reserved_regions = get_regions_from_reg(*reserved_node);
                    for (auto region : reserved_regions) {
                        regions.push_back(region);
                    }
                }
            }
        }
    }
    return regions;
}

using PROBE_DBG = DebugScope<"Probe", true>;

struct dev_tree::probe_ctx {
    bek::span<ProbeFn*> probe_functions;
    bek::vector<Node*> waiting{};
};

dev_tree::DevStatus dev_tree::probe_node(dev_tree::Node& node, dev_tree::device_tree& tree, dev_tree::probe_ctx& ctx) {
    for (auto probe_fn : ctx.probe_functions) {
        auto res = probe_fn(node, tree, ctx);
        if (res != dev_tree::DevStatus::Unrecognised) {
            node.nodeStatus = res;

            if (res == dev_tree::DevStatus::Waiting) {
                PROBE_DBG::dbgln("Device {} Waiting."_sv, node.name);
                ctx.waiting.push_back(&node);
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

void dev_tree::probe_nodes(device_tree& tree, bek::span<ProbeFn*> probe_functions) {
    auto& root_node = *tree.root_node;
    PROBE_DBG::dbgln("Probing DeviceTree."_sv);

    auto ctx = probe_ctx{probe_functions, {}};

    probe_node(root_node, tree, ctx);
    uSize last_waiting = 0;
    for (int iterations = 3; iterations > 0; (ctx.waiting.size() != last_waiting) || iterations--) {
        last_waiting = ctx.waiting.size();
        if (last_waiting) {
            PROBE_DBG::dbgln("Clearing {} waiting nodes."_sv, ctx.waiting.size());
            auto vec = bek::move(ctx.waiting);
            for (auto node : vec) {
                probe_node(*node, tree, ctx);
            }
        } else {
            PROBE_DBG::dbgln("All Nodes Probed."_sv);
            break;
        }
    }
}

bek::optional<bek::buffer> Node::get_property(bek::str_view name) const {
    for (auto& prop : properties) {
        if (prop.name == name) {
            return {prop.data};
        }
    }
    return {};
}
