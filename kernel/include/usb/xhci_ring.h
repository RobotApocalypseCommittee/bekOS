// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BEKOS_XHCI_RING_H
#define BEKOS_XHCI_RING_H

#include <usb/usb.h>

#include "bek/array.h"
#include "bek/format_core.h"
#include "bek/optional.h"
#include "library/function.h"
#include "mm/addresses.h"
#include "mm/dma_utils.h"
namespace xhci {

enum class TRBType : u8 {
    Normal          = 1,
    Setup           = 2,
    Data            = 3,
    Status          = 4,
    Isoch           = 5,
    Link            = 6,
    Event           = 7,
    NoOp            = 8,
    EnableSlot      = 9,
    DisableSlot     = 10,
    AddressDevice   = 11,
    ConfigEndpoint  = 12,
    EvaluateContext = 13,
    ResetEndpoint   = 14,
    StopEndpoint    = 15,
    SetTRDequeuePtr = 16,
    ResetDevice     = 17,
    // Force Event
    NegBandwidth = 19,
    // Set Latency Tolerance
    GetPortBandwidth = 21,
    ForceHeader      = 22,
    NoOpCommand      = 23,
    // Blah
    TransferEvent     = 32,
    CommandCompletion = 33,
    PortStatusChange  = 34,
    BandwidthRequest  = 35,
    // Doorbell Event
    HostControllerEvent = 37,
    DeviceNotification  = 38,
    MFINDEXWrap         = 39,
};

struct TRB {
    u32 data[4];

    u64 parameter() const { return static_cast<u64>(data[0]) | static_cast<u64>(data[1]) << 32; }
    void set_parameter(u64 parameter) {
        data[0] = parameter & 0xFFFFFFFF;
        data[1] = parameter >> 32;
    }

    u32& status() { return data[2]; }
    TRBType trb_type() const { return static_cast<TRBType>((data[3] >> 10) & 0x3F); }
    void trb_type(TRBType t) {
        data[3] = (data[3] & ~(0x3F << 10)) | ((static_cast<u32>(t) & 0x3F) << 10);
    }

    bool cycle() const { return data[3] & 1; }
    void cycle(bool c) { data[3] = (data[3] & ~1) | (c ? 1 : 0); }

    static TRB create_address_dev_cmd(u64 in_ctx_ptr, u8 slot_id, bool make_request = true) {
        TRB trb{0, 0, 0, 0};
        trb.set_parameter(in_ctx_ptr);
        trb.trb_type(TRBType::AddressDevice);
        trb.data[3] |= static_cast<u32>(slot_id) << 24;
        if (!make_request) trb.data[3] |= (1u << 9);
        return trb;
    }

    constexpr static TRB create(TRBType kind, u64 parameter, u32 status, u32 control) {
        return {static_cast<u32>(parameter & 0xFFFFFFFF), static_cast<u32>(parameter >> 32), status,
                (control & ~(0x3Fu << 10)) | ((static_cast<u32>(kind) & 0x3F) << 10)};
    }
    constexpr static TRB create(TRBType kind, u32 parameter0, u32 parameter1, u32 status,
                                u32 control) {
        return {parameter0, parameter1, status,
                (control & ~(0x3Fu << 10)) | ((static_cast<u32>(kind) & 0x3F) << 10)};
    }
};
static_assert(sizeof(TRB) == 16);

namespace command {
inline TRB address_device(uPtr in_ctx_dma, u8 slot_id, bool make_request = true) {
    TRB trb{0, 0, 0, 0};
    trb.set_parameter(in_ctx_dma);
    trb.trb_type(TRBType::AddressDevice);
    trb.data[3] |= static_cast<u32>(slot_id) << 24;
    if (!make_request) trb.data[3] |= (1u << 9);
    return trb;
}

inline TRB configure_endpoint(uPtr in_ctx_dma, u8 slot_id, bool deconfigure = false) {
    return TRB::create(TRBType::ConfigEndpoint, in_ctx_dma, 0,
                       static_cast<u32>(slot_id) << 24 | (deconfigure ? (1u << 9) : 0));
}

}  // namespace command

struct EventTRB {
    union {
        /// Present for Command Completion and Transfer events.
        u64 trb_ptr;
        /// Present for notification event.
        u64 notification_data;
    };
    union {
        /// Present for Transfer event.
        u32 transfer_length;
        /// Present for Command Completion event.
        u32 completion_parameter;
    };
    TRBType kind;
    /// Present on all events.
    u8 completion_code;
    /// Present on most events
    u8 slot_id;

    union {
        /// Present for port status change event.
        u8 port_id;
        /// Present for command completion event + doorbell event
        u8 vf_id;
        /// Present for transfer event.
        u8 endpoint_id;
        /// Present for notification event.
        u8 notification_type;
    };

    /// Present for transfer event.
    bool ed_flag;

    static EventTRB fromTRB(TRB trb);
};

void bek_basic_format(bek::OutputStream&, const TRBType&);

class ProducerRing {
    // Not entirely sure why - but hey. See managarm.
    constexpr static uSize RING_SIZE = 128;

public:
    using Callback = bek::function<void(EventTRB), false, true>;

    ProducerRing(mem::dma_pool& pool) : m_ring_array(pool, RING_SIZE, 64), m_enqueue_index{0} {
        for (auto& trb : m_ring_array) {
            trb = {0, 0, 0, 0};
        }
        m_ring_array.sync_after_write();
    }
    /// Returns pointer to bottom of ring.
    mem::DmaPtr dma_ptr() const { return m_ring_array.dma_ptr(); }
    void push_command(TRB raw_trb, Callback callback);
    void process_completion(EventTRB trb);

    void push_control_transfer(usb::SetupPacket packet, mem::dma_buffer data_ref,
                               Callback&& callback);

private:
    mem::dma_array<TRB> m_ring_array;
    uSize m_enqueue_index;
    bool m_current_pcs{true};
    bek::array<Callback, RING_SIZE> m_completions{};
};

class EventRing {
    constexpr static uSize RING_SIZE = 128;

public:
    explicit EventRing(mem::dma_pool& pool)
        : m_ring_array(pool, RING_SIZE, 64), m_erst{pool}, m_dequeue_index{0}, current_ccs{true} {
        // Clear ring
        for (auto& trb : m_ring_array) {
            trb = {0, 0, 0, 0};
        }

        // Setup ERST array

        auto ring_array_ptr = m_ring_array.dma_ptr().get();
        *m_erst             = {static_cast<u32>(ring_array_ptr & 0xFFFFFFFF),
                               static_cast<u32>(ring_array_ptr >> 32), RING_SIZE, 0};
        m_erst.sync_after_write();
    }

    /// Tries to process a TRB and advance pointer. Caller should repeat until returns none, at
    /// which point update ERDP.
    bek::optional<EventTRB> process() {
        m_ring_array.sync_before_read(m_dequeue_index);
        auto trb = m_ring_array[m_dequeue_index];
        if (trb.cycle() == current_ccs) {
            // Is a valid trb.
            m_dequeue_index++;
            if (m_dequeue_index == RING_SIZE) {
                m_dequeue_index = 0;
                current_ccs     = !current_ccs;
            }

            return EventTRB::fromTRB(trb);
        } else {
            return {};
        }
    }

    mem::DmaPtr erst_dma_ptr() const { return m_erst.dma_ptr(); }
    uSize erst_size() const { return 1; }
    mem::DmaPtr current_ring_dma_ptr() const { return m_ring_array.dma_ptr(m_dequeue_index); }

private:
    struct alignas(64) ERSTEntry {
        u32 ring_segment_base_low;
        u32 ring_segment_base_high;
        u32 ring_segment_size;
        u32 _reserved;
    };
    mem::dma_array<TRB> m_ring_array;
    mem::dma_object<ERSTEntry> m_erst;
    uSize m_dequeue_index;
    bool current_ccs;
};

namespace transfer {

constexpr TRB make_setup(usb::SetupPacket packet, bool data_stage, bool in) {
    // TODO: Interrupter Target?

    return TRB::create(
        TRBType::Setup,
        static_cast<u32>(packet.request_type) | (static_cast<u32>(packet.request) << 8) |
            (static_cast<u32>(packet.value) << 16),
        static_cast<u32>(packet.index) | (static_cast<u32>(packet.data_length) << 16), 8,
        (1u << 6) | ((data_stage ? (in ? 3u : 2u) : 0u) << 16));
}

constexpr TRB make_data_stage(uPtr data_dma_ptr, uSize transfer_length, bool chain, bool data_in) {
    VERIFY(transfer_length <= 64 * 1024);
    // TODO: Fill in?
    u32 td_len = 0;
    return TRB::create(TRBType::Data, data_dma_ptr,
                       static_cast<u32>((transfer_length & 0x1FFFF) | ((td_len & 0b11111) << 17)),
                       (chain ? (1u << 4) : 0) | (data_in ? (1u << 16) : 0));
}

constexpr TRB make_status(bool status_in, bool ioc) {
    return TRB::create(TRBType::Status, 0, 0, (status_in ? (1u << 16) : 0) | (ioc ? (1u << 5) : 0));
}

}  // namespace transfer

}  // namespace xhci

#endif  // BEKOS_XHCI_RING_H
