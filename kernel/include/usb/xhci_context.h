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

#ifndef BEKOS_XHCI_CONTEXT_H
#define BEKOS_XHCI_CONTEXT_H

enum class EndpointType : u8 {
    Invalid = 0,
    IsochOut,
    BulkOut,
    InterruptOut,
    Control,
    IsochIn,
    BulkIn,
    InterruptIn,
};

constexpr EndpointType ep_type_from(usb::TransferType ttype, usb::Direction dir) {
    using enum usb::TransferType;
    switch (ttype) {
        case Control:
            return EndpointType::Control;
        case Isochronous:
            return dir == usb::Direction::In ? EndpointType::IsochIn : EndpointType::IsochOut;
        case Bulk:
            return dir == usb::Direction::In ? EndpointType::BulkIn : EndpointType::BulkOut;
        case Interrupt:
            return dir == usb::Direction::In ? EndpointType::InterruptIn
                                             : EndpointType::InterruptOut;
    }
    ASSERT_UNREACHABLE();
}

struct RawContext {
    u32 data[8];
};

struct SlotContext {
    u32 data[8];

    u32 get_route_string() const { return data[0] & 0xFFFFF; }
    void set_route_string(u32 s) { data[0] = (data[0] & 0xFFF000000) | (s & 0xFFFFF); }
    bool get_multi_tt() const { return data[0] & (1u << 25); }
    void set_multi_tt(bool b) { data[0] = (data[0] & ~(1u << 25)) | (b ? (1u << 25) : 0); }
    bool get_hub() const { return data[0] & (1u << 26); }
    void set_hub(bool b) { data[0] = (data[0] & ~(1u << 26)) | (b ? (1u << 26) : 0); }
    u8 get_context_entries() const { return (data[0] >> 27) & 0b11111; }
    void set_context_entries(u8 n) {
        data[0] = (data[0] & 0x07FFFFFF) | (static_cast<u32>(n & 0b11111) << 27);
    }

    u16 get_max_exit_latency() const { return data[1] & 0xFFFF; }
    void set_max_exit_latency(u16 l) { data[1] = (data[1] & ~0xFFFF) | l; }
    u8 get_root_hub_port() const { return (data[1] >> 16) & 0xFF; }
    void set_root_hub_port(u8 n) { data[1] = (data[1] & 0xFF00FFFF) | ((u32)n << 16); }
    u8 get_port_number() const { return (data[1] >> 24) & 0xFF; }
    void set_port_number(u8 n) { data[1] = (data[1] & 0x00FFFFFF) | ((u32)n << 24); }

    u8 get_parent_hub_slot() const { return data[2] & 0xFF; }
    void set_parent_hub_slot(u8 s) { data[2] = (data[2] & 0xFFFFFF00) | s; }
    u8 get_parent_port() const { return (data[2] >> 8) & 0xFF; }
    void set_parent_port(u8 n) { data[2] = (data[2] & 0xFFFF00FF) | ((u32)n << 8); }
    u8 get_tt_think_time() const { return (data[2] >> 16) & 0b11; }
    void set_tt_think_time(u8 n) { data[2] = (data[2] & 0xFFF0FFFF) | ((u32)n << 16); }
    u16 get_interrupter_target() const { return data[2] >> 22; }
    void set_interrupter_target(u16 t) { data[2] = (data[2] & 0x003FFFFF) | ((u32)t << 22); }

    u8 get_device_address() const { return data[3] & 0xFF; }
    // set this
    u8 get_slot_state() const { return data[3] >> 27; }
};

struct InputControlContext {
    u32 data[8];

    bool get_drop_flag(u8 n) const { return (data[0] >> n) & 1; }
    void set_drop_flag(u8 n, bool b) { data[0] = (data[0] & ~(1u << n)) | (b ? (1u << n) : 0); }
    bool get_add_flag(u8 n) const { return (data[1] >> n) & 1; }
    void set_add_flag(u8 n, bool b) { data[1] = (data[1] & ~(1u << n)) | (b ? (1u << n) : 0); }

    u8 get_config_value() const { return data[7] & 0xFF; }
    void set_config_value(u8 s) { data[2] = (data[7] & 0xFFFFFF00) | s; }
    u8 get_interface_number() const { return (data[7] >> 8) & 0xFF; }
    void set_interface_number(u8 n) { data[7] = (data[7] & 0xFFFF00FF) | ((u32)n << 8); }
    u8 get_alternate_setting() const { return (data[7] >> 16) & 0xFF; }
    void set_alternate_setting(u8 n) { data[7] = (data[7] & 0xFF00FFFF) | ((u32)n << 16); }
};

struct EndpointContext {
    u32 data[8];

    u8 get_endpoint_state() const { return data[0] & 0b111; }
    void set_endpoint_state(u8 s) { data[0] = (data[0] & ~0b111) | (s & 0b111); }
    // TODO: Mult, MaxPStreams, LSA?
    u8 get_interval() const { return (data[0] >> 16) & 0xFF; }
    void set_interval(u8 i) { data[0] = (data[0] & 0xFF00FFFF) | (i << 16); }
    // TODO: ESIT?

    u8 get_error_count() const { return (data[1] >> 1) & 0b11; }
    void set_error_count(u8 c) { data[1] = (data[1] & ~0b110) | ((c & 0b11) << 1); }
    EndpointType get_endpoint_type() const {
        return static_cast<EndpointType>((data[1] >> 3) & 0b111);
    }
    void set_endpoint_type(EndpointType t) {
        data[1] = (data[1] & ~0b111000) | (static_cast<u8>(t) << 3);
    }
    // TODO: HID?
    u8 get_max_burst_size() const { return (data[1] >> 8) & 0xFF; }
    void set_max_burst_size(u8 s) { data[1] = (data[1] & 0xFFFF00FF) | ((u32)s << 8); }

    u16 get_max_packet_size() const { return data[1] >> 16; }
    void set_max_packet_size(u16 s) { data[1] = (data[1] & 0xFFFF) | ((u32)s << 16); }

    void set_dequeue_ptr(u64 ptr, bool cycle) {
        data[3] = ptr >> 32;
        data[2] = (ptr & 0xFFFFFFFF) | (cycle ? 1 : 0);
    }

    u16 get_avg_trb_length() const { return data[4] & 0xFFFF; }
    void set_avg_trb_length(u16 l) { data[4] = (data[4] & 0xFFFF0000) | l; }
};

constexpr inline uSize input_context_size  = 34;
constexpr inline uSize device_context_size = 32;

struct ContextArray {
    enum EndpointDirection { In, Out, Control };
    static constexpr uSize control_ici = 0;
    static constexpr uSize slot_ici    = 1;
    static constexpr uSize ep_ici(uSize endpoint_idx) { return endpoint_idx + 2; }
    static constexpr uSize slot_dci = 0;
    static constexpr uSize ep_dci(uSize endpoint_idx) { return endpoint_idx + 1; }

    // FIXME: Cannot cross a page boundary - easiest just to align?
    ContextArray(mem::dma_pool& pool, uSize n_contexts, bool large)
        : m_array(pool, n_contexts * (large ? 2 : 1), 64), m_large{large} {
        for (auto& ctx : m_array) {
            ctx = {0, 0, 0, 0, 0, 0, 0, 0};
        }
    }

    template <typename T>
        requires bek::same_as<T, RawContext> || bek::same_as<T, InputControlContext> ||
                 bek::same_as<T, SlotContext> || bek::same_as<T, EndpointContext>
    T& get_as(uSize idx) {
        idx = idx * (m_large ? 2 : 1);
        VERIFY(idx < m_array.size());
        return reinterpret_cast<T&>(m_array[idx]);
    }

    void sync_before_read(uSize specific_index = -1ul) {
        if (specific_index != -1ul && m_large) specific_index *= 2;
        m_array.sync_before_read(specific_index);
    }

    void sync_after_write(uSize specific_index = -1ul) {
        if (specific_index != -1ul && m_large) specific_index *= 2;
        m_array.sync_after_write(specific_index);
    }

    uPtr dma_ptr() const { return m_array.dma_ptr().get(); }

private:
    mem::dma_array<RawContext> m_array;
    bool m_large;
};

#endif  // BEKOS_XHCI_CONTEXT_H
