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

#include "usb/xhci_ring.h"

#include "bek/assertions.h"
#include "library/debug.h"

using DBG = DebugScope<"xHCI", DebugLevel::WARN>;

xhci::EventTRB xhci::EventTRB::fromTRB(xhci::TRB trb) {
    EventTRB event{};
    event.kind            = static_cast<TRBType>(trb.trb_type());
    event.completion_code = trb.status() >> 24 & 0xFF;
    u32 lower_status      = trb.status() & 0x00FFFFFF;
    u8 upper_control      = trb.data[3] >> 24 & 0xFF;
    u8 mid_control        = trb.data[3] >> 16 & 0xFF;

    switch (event.kind) {
        case TRBType::TransferEvent:
            event.trb_ptr         = trb.parameter();
            event.transfer_length = lower_status;
            event.slot_id         = upper_control;
            event.endpoint_id     = mid_control & 0x1F;
            event.ed_flag         = (trb.data[3] >> 2) & 1;
            break;
        case TRBType::CommandCompletion:
            event.trb_ptr              = trb.parameter();
            event.completion_parameter = lower_status;
            event.slot_id              = upper_control;
            event.vf_id                = mid_control;
            break;
        case TRBType::PortStatusChange:
            event.port_id = trb.data[0] >> 24 & 0xFF;
            break;
        case TRBType::BandwidthRequest:
            event.slot_id = upper_control;
            break;
        case TRBType::HostControllerEvent:
            break;
        case TRBType::DeviceNotification:
            event.notification_data = trb.parameter() & ~static_cast<u64>(0xFF);
            event.notification_type = trb.parameter() >> 4 & 0xF;
            event.slot_id           = upper_control;
            break;
        case TRBType::MFINDEXWrap:
            break;
        default:
            ASSERT(false);
    }
    return event;
}

void xhci::bek_basic_format(bek::OutputStream &out, const xhci::TRBType &kind) {
    switch (kind) {
        case TRBType::Normal:
            out.write("Normal"_sv);
            break;
        case TRBType::Setup:
            out.write("Setup"_sv);
            break;
        case TRBType::Data:
            out.write("Data"_sv);
            break;
        case TRBType::Status:
            out.write("Status"_sv);
            break;
        case TRBType::Isoch:
            out.write("Isoch"_sv);
            break;
        case TRBType::Link:
            out.write("Link"_sv);
            break;
        case TRBType::Event:
            out.write("Event"_sv);
            break;
        case TRBType::NoOp:
            out.write("NoOp"_sv);
            break;
        case TRBType::EnableSlot:
            out.write("EnableSlot"_sv);
            break;
        case TRBType::DisableSlot:
            out.write("DisableSlot"_sv);
            break;
        case TRBType::AddressDevice:
            out.write("AddressDevice"_sv);
            break;
        case TRBType::ConfigEndpoint:
            out.write("ConfigEndpoint"_sv);
            break;
        case TRBType::EvaluateContext:
            out.write("EvaluateContext"_sv);
            break;
        case TRBType::ResetEndpoint:
            out.write("ResetEndpoint"_sv);
            break;
        case TRBType::StopEndpoint:
            out.write("StopEndpoint"_sv);
            break;
        case TRBType::SetTRDequeuePtr:
            out.write("SetTRDequeuePtr"_sv);
            break;
        case TRBType::ResetDevice:
            out.write("ResetDevice"_sv);
            break;
        case TRBType::NegBandwidth:
            out.write("NegBandwidth"_sv);
            break;
        case TRBType::GetPortBandwidth:
            out.write("GetPortBandwidth"_sv);
            break;
        case TRBType::ForceHeader:
            out.write("ForceHeader"_sv);
            break;
        case TRBType::NoOpCommand:
            out.write("NoOpCommand"_sv);
            break;
        case TRBType::TransferEvent:
            out.write("TransferEvent"_sv);
            break;
        case TRBType::CommandCompletion:
            out.write("CommandCompletion"_sv);
            break;
        case TRBType::PortStatusChange:
            out.write("PortStatusChange"_sv);
            break;
        case TRBType::BandwidthRequest:
            out.write("BandwidthRequest"_sv);
            break;
        case TRBType::HostControllerEvent:
            out.write("HostControllerEvent"_sv);
            break;
        case TRBType::DeviceNotification:
            out.write("DeviceNotification"_sv);
            break;
        case TRBType::MFINDEXWrap:
            out.write("MFINDEXWrap"_sv);
            break;
        default:
            out.write("AAAAAAA"_sv);
            break;
    }
}
void xhci::ProducerRing::push_command(xhci::TRB raw_trb, xhci::ProducerRing::Callback callback) {
    m_completions[m_enqueue_index] = bek::move(callback);
    raw_trb.cycle(m_current_pcs);
    VERIFY(m_ring_array[m_enqueue_index].cycle() != m_current_pcs);
    m_ring_array[m_enqueue_index] = raw_trb;
    m_ring_array.sync_after_write(m_enqueue_index);
    m_enqueue_index++;

    if (m_enqueue_index == RING_SIZE - 1) {
        uSize dma_ptr                 = m_ring_array.dma_ptr().get();
        m_ring_array[m_enqueue_index] = {
            static_cast<u32>(dma_ptr), static_cast<u32>((dma_ptr >> 32)), 0,
            static_cast<u32>(TRBType::Link) << 10 | (1u << 1) | m_current_pcs};
        m_ring_array.sync_after_write(m_enqueue_index);
        m_enqueue_index = 0;
        m_current_pcs   = !m_current_pcs;
    }
}
void xhci::ProducerRing::process_completion(xhci::EventTRB trb) {
    VERIFY(trb.kind == TRBType::CommandCompletion || trb.kind == TRBType::TransferEvent);
    uSize index = (trb.trb_ptr - dma_ptr().get()) / sizeof(TRB);
    VERIFY(index < RING_SIZE);

    Callback cb{};
    bek::swap(cb, m_completions[index]);
    if (cb) {
        cb(trb);
    } else {
        DBG::dbgln("Interrupt without completion, idx {}"_sv, index);
    }
}
void xhci::ProducerRing::push_control_transfer(usb::SetupPacket packet, mem::dma_buffer data_ref,
                                               xhci::ProducerRing::Callback &&callback) {
    using namespace transfer;
    bool data_stage = data_ref.size();
    bool data_in    = packet.get_direction() == usb::Direction::In;
    bool status_in  = data_ref.size() == 0 || packet.get_direction() == usb::Direction::Out;

    bool callback_present = !!callback;

    // TODO: longer data stages.
    VERIFY(data_ref.size() <= 64 * KiB);
    VERIFY(data_ref.size() == packet.data_length);
    push_command(make_setup(packet, data_stage, data_in), {});

    if (data_stage) {
        push_command(make_data_stage(data_ref.dma_ptr().get(), data_ref.size(), false, data_in),
                     {});
    }
    push_command(make_status(status_in, callback_present), bek::move(callback));
}
