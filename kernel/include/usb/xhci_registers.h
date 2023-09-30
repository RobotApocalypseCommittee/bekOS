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

#ifndef BEKOS_XHCI_REGISTERS_H
#define BEKOS_XHCI_REGISTERS_H

#define BEK_SET_BIT(VALUE, POS) (VALUE | (1U << POS))
#define BEK_CLEAR_BIT(VALUE, POS) (VALUE & (~(1U << POS)))
#define BEK_BIT_MASK(BITS) ((1u << BITS) - 1)

#define BEK_BIT_PRESERVE_ALL 0xFFFFFFFF
#define BEK_BIT_PRESERVE_NONE 0

#define BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS) \
    bool get_##NAME() const { return (REGISTER() >> POS) & 1; }

#define BEK_BIT_ACCESSOR_RW1S(NAME, REGISTER, POS) \
    BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS)       \
    void set_##NAME() { REGISTER(BEK_SET_BIT(REGISTER##_preserved(), POS)); }

#define BEK_BIT_ACCESSOR_RW1C(NAME, REGISTER, POS) \
    BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS)       \
    void clear_##NAME() { REGISTER(BEK_SET_BIT(REGISTER##_preserved(), POS)); }

#define BEK_BIT_ACCESSOR_RW(NAME, REGISTER, POS)                              \
    BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS)                                  \
    void set_##NAME() { REGISTER(BEK_SET_BIT(REGISTER##_preserved(), POS)); } \
    void clear_##NAME() { REGISTER(BEK_CLEAR_BIT(REGISTER##_preserved(), POS)); }

#define BEK_REGISTER_RO(NAME, TYPE, OFFSET) \
    TYPE NAME() const { return base.read<TYPE>(OFFSET); }

#define BEK_REGISTER_P(NAME, TYPE, OFFSET, PRESERVE_MASK) \
    BEK_REGISTER_RO(NAME, TYPE, OFFSET)                   \
    void NAME(TYPE v) { base.write<TYPE>(OFFSET, v); }    \
    TYPE NAME##_preserved() const { return base.read<TYPE>(OFFSET) & PRESERVE_MASK; }

#define BEK_REGISTER(NAME, TYPE, OFFSET) BEK_REGISTER_P(NAME, TYPE, OFFSET, BEK_BIT_PRESERVE_ALL)

#define BEK_BIT_FIELD_RO(NAME, REGISTER, POS, BITS, TYPE) \
    [[nodiscard]] TYPE get_##NAME() const { return (REGISTER() >> POS) & BEK_BIT_MASK(BITS); }

#define BEK_BIT_FIELD_RW(NAME, REGISTER, POS, BITS, TYPE)                  \
    BEK_BIT_FIELD_RO(NAME, REGISTER, POS, BITS, TYPE)                      \
    void set_##NAME(TYPE v) {                                              \
        REGISTER((REGISTER##_preserved() & ~(BEK_BIT_MASK(BITS) << POS)) | \
                 (static_cast<u32>(v) << POS));                            \
    }

struct CapabilityRegisters {
    constexpr static uSize BANK_SIZE = 0x20;

    BEK_REGISTER_RO(CAPLENGTH, u8, 0)
    BEK_REGISTER_RO(HCIVERSION, u16, 2)
    BEK_REGISTER_RO(HCSPARAMS1, u32, 4)
    BEK_REGISTER_RO(HCSPARAMS2, u32, 8)
    BEK_REGISTER_RO(HCSPARAMS3, u32, 12)
    BEK_REGISTER_RO(HCCPARAMS1, u32, 16)
    BEK_REGISTER_RO(DBOFF, u32, 20)
    BEK_REGISTER_RO(RTSOFF, u32, 24)
    BEK_REGISTER_RO(HCCPARAMS2, u32, 28)

    u8 max_device_slots() const { return HCSPARAMS1() & 0xFF; }
    u16 max_interrupters() const {
        // Bits 18:8 (11)
        return (HCSPARAMS1() >> 8) & 0x7FF;
    }
    u8 max_ports() const { return (HCSPARAMS1() >> 24) & 0xFF; }

    u8 ist() const { return HCSPARAMS2() & 0xF; }

    u8 erst_max() const { return (HCSPARAMS2() >> 4) & 0xF; }

    u16 max_scratchpad_buffers() const {
        auto hcsparams2 = HCSPARAMS2();
        auto hi         = (hcsparams2 >> 21) & 0x1F;
        auto lo         = (hcsparams2 >> 27) & 0x1F;
        return (hi << 5) | lo;
    }

    bool scratchpad_restore() const { return (HCSPARAMS2() >> 26) & 1; }

    u8 u1_dev_exit_latency() const { return HCSPARAMS3() & 0xFF; }

    u16 u2_dev_exit_latency() const { return (HCSPARAMS3() >> 16) & 0xFFFF; }

    bool supports_64bit_addressing() const { return (HCCPARAMS1() & 1); }

    bool supports_bandwidth_negotiation() const { return (HCCPARAMS1() >> 1) & 1; }

    bool context_64bit() const { return (HCCPARAMS1() >> 2) & 1; }

    bool ppc() const { return (HCCPARAMS1() >> 3) & 1; }

    bool pind() const { return (HCCPARAMS1() >> 4) & 1; }

    bool lhrc() const { return (HCCPARAMS1() >> 5) & 1; }

    bool ltc() const { return (HCCPARAMS1() >> 6) & 1; }

    bool nss() const { return (HCCPARAMS1() >> 7) & 1; }

    bool pae() const { return (HCCPARAMS1() >> 8) & 1; }

    bool spc() const { return (HCCPARAMS1() >> 9) & 1; }

    bool sec() const { return (HCCPARAMS1() >> 10) & 1; }

    bool cfc() const { return (HCCPARAMS1() >> 11) & 1; }

    u8 max_psa_size() const { return (HCCPARAMS1() >> 12) & 0xF; }

    mem::PCIeDeviceArea extended_cap_ptr() const {
        uSize offset = (HCCPARAMS1() >> 16) << 2;
        return base.subdivide(offset, base.size() - offset);
    }

    uSize runtime_register_offset() const { return RTSOFF() & ~BEK_BIT_MASK(5); }

    bool u3c() const { return (HCCPARAMS2() >> 0) & 1; }

    bool cmc() const { return (HCCPARAMS2() >> 1) & 1; }

    bool fsc() const { return (HCCPARAMS2() >> 2) & 1; }

    bool ctc() const { return (HCCPARAMS2() >> 3) & 1; }

    bool lec() const { return (HCCPARAMS2() >> 4) & 1; }

    bool cic() const { return (HCCPARAMS2() >> 5) & 1; }

    bool etc() const { return (HCCPARAMS2() >> 6) & 1; }

    bool etc_tsc() const { return (HCCPARAMS2() >> 7) & 1; }

    bool gsc() const { return (HCCPARAMS2() >> 8) & 1; }

    bool vtc() const { return (HCCPARAMS2() >> 9) & 1; }

    mem::PCIeDeviceArea base;
};

struct PortOperationalRegisters {
    BEK_REGISTER_P(PORTSC, u32, 0, 0b00001110000000011100001111100000)
    BEK_REGISTER(PORTPMSC, u32, 4)
    BEK_REGISTER(PORTLI, u32, 8)
    BEK_REGISTER(PORTHLPMC, u32, 12)

    BEK_BIT_ACCESSOR_RO(connect_status, PORTSC, 0)
    BEK_BIT_ACCESSOR_RW1C(port_enabled, PORTSC, 1)
    BEK_BIT_ACCESSOR_RO(overcurrent_active, PORTSC, 3)
    BEK_BIT_ACCESSOR_RW1S(port_reset, PORTSC, 4)

    enum PortLinkState : u8 {
        U0 = 0,
        /// Read only
        U1,
        U2,
        U3,
        // Read only
        Disabled,
        RxDetect,
        /// Read only
        Inactive,
        /// Read only
        Polling,
        /// Read only
        Recover,
        /// Read only
        HotReset,
        Compliance,
        // Read Only
        Test,
        Resume = 15
    };

    [[nodiscard]] PortLinkState get_port_link_state() const {
        return static_cast<PortLinkState>((PORTSC() >> 5) & 0xF);
    }

    void set_port_link_state(PortLinkState state) {
        // Need to set LWS as well.
        PORTSC((PORTSC_preserved() & ~(0xF << 5)) | (static_cast<u32>(state) << 5) | (1u << 16));
    }

    BEK_BIT_ACCESSOR_RW(port_power, PORTSC, 9)

    u8 get_port_speed() const { return (PORTSC() >> 10) & 0xF; }

    enum PortIndicatorControl : u8 { Off = 0, Amber = 1, Green = 2, Undefined = 3 };

    PortIndicatorControl get_port_indicator_control() const {
        return static_cast<PortIndicatorControl>((PORTSC() >> 14) & 0b11);
    }

    void set_port_indicator_control(PortIndicatorControl control) {
        PORTSC((PORTSC_preserved() & ~(0b11 << 14)) | (static_cast<u32>(control) << 14));
    }

    BEK_BIT_ACCESSOR_RW1C(connect_status_change, PORTSC, 17)
    BEK_BIT_ACCESSOR_RW1C(port_enabled_change, PORTSC, 18)
    BEK_BIT_ACCESSOR_RW1C(warm_port_reset_change, PORTSC, 19)
    BEK_BIT_ACCESSOR_RW1C(overcurrent_change, PORTSC, 20)
    BEK_BIT_ACCESSOR_RW1C(port_reset_change, PORTSC, 21)
    BEK_BIT_ACCESSOR_RW1C(port_link_state_change, PORTSC, 22)
    BEK_BIT_ACCESSOR_RW1C(port_config_error_change, PORTSC, 23)
    BEK_BIT_ACCESSOR_RO(cold_attach_status, PORTSC, 24)
    BEK_BIT_ACCESSOR_RW(wake_on_connect_enable, PORTSC, 25)
    BEK_BIT_ACCESSOR_RW(wake_on_connect_disable, PORTSC, 26)
    BEK_BIT_ACCESSOR_RW(wake_on_overcurrent_enable, PORTSC, 27)
    BEK_BIT_ACCESSOR_RO(device_removeable, PORTSC, 30)
    BEK_BIT_ACCESSOR_RW1S(warm_port_reset, PORTSC, 31)

    u8 get_usb3_u1_timeout() const { return PORTPMSC() & 0xFF; }
    u8 get_usb3_u2_timeout() const { return (PORTPMSC() >> 8) & 0xFF; }
    // TODO: Writes

    BEK_BIT_ACCESSOR_RW(usb3_force_link_pm_accept, PORTPMSC, 16)

    enum L1Status {
        Invalid      = 0,
        Success      = 1,
        NotYet       = 2,
        NotSupported = 3,
        TimeoutError = 4,
    };
    L1Status get_usb2_l1_status() const { return static_cast<L1Status>(PORTPMSC() & 0b111); }

    BEK_BIT_ACCESSOR_RW(usb2_remote_wake_enable, PORTPMSC, 3)
    BEK_BIT_FIELD_RW(usb2_best_effort_service_latency, PORTPMSC, 4, 4, u8)
    BEK_BIT_FIELD_RW(usb2_L1_device_slot, PORTPMSC, 8, 8, u8)
    BEK_BIT_ACCESSOR_RW(usb2_hardware_lpm_enable, PORTPMSC, 16)

    // PORTLI
    BEK_BIT_FIELD_RW(usb3_link_error_count, PORTLI, 0, 16, u16)
    BEK_BIT_FIELD_RO(usb3_rx_lane_count, PORTLI, 16, 4, u8)
    BEK_BIT_FIELD_RO(usb3_tx_lane_count, PORTLI, 20, 4, u8)

    // PORTHLPMC

    mem::PCIeDeviceArea base;
};

struct OperationalRegisters {
    constexpr static uSize BANK_SIZE = 0x1400;
    BEK_REGISTER(USBCMD, u32, 0)

    BEK_REGISTER_P(USBSTS, u32, 0x04, BEK_BIT_PRESERVE_NONE)
    BEK_REGISTER_RO(PAGESIZE, u32, 0x08)
    BEK_REGISTER(DNCTRL, u32, 0x14)
    BEK_REGISTER_P(CRCR, u64, 0x18, 0xFFFFFFFFFFFFFFFF)
    BEK_REGISTER(DCBAAP, u64, 0x30)
    BEK_REGISTER(CONFIG, u32, 0x38)

    BEK_BIT_ACCESSOR_RW(run_stop, USBCMD, 0)
    BEK_BIT_ACCESSOR_RW(hc_reset, USBCMD, 1)
    BEK_BIT_ACCESSOR_RW(interrupter_enable, USBCMD, 2)
    BEK_BIT_ACCESSOR_RW(host_error_enable, USBCMD, 3)
    BEK_BIT_ACCESSOR_RW(light_hc_reset, USBCMD, 7)
    BEK_BIT_ACCESSOR_RW(hc_save_state, USBCMD, 8)
    BEK_BIT_ACCESSOR_RW(hc_restore_state, USBCMD, 9)
    BEK_BIT_ACCESSOR_RW(event_wrap_enable, USBCMD, 10)
    BEK_BIT_ACCESSOR_RW(u3_mfindex_stop_enable, USBCMD, 11)
    BEK_BIT_ACCESSOR_RW(cem_enable, USBCMD, 13)
    BEK_BIT_ACCESSOR_RW(ext_tbc_enable, USBCMD, 14)
    BEK_BIT_ACCESSOR_RW(ext_tbc_trb_stat_enable, USBCMD, 15)
    BEK_BIT_ACCESSOR_RW(vtio_enable, USBCMD, 16)

    BEK_BIT_ACCESSOR_RO(hc_halted, USBSTS, 0)
    BEK_BIT_ACCESSOR_RW1C(host_error, USBSTS, 2)
    BEK_BIT_ACCESSOR_RW1C(event_interrupt, USBSTS, 3)
    BEK_BIT_ACCESSOR_RW1C(port_change_detected, USBSTS, 4)
    BEK_BIT_ACCESSOR_RO(save_state_stat, USBSTS, 8)
    BEK_BIT_ACCESSOR_RO(restore_state_stat, USBSTS, 9)
    BEK_BIT_ACCESSOR_RW1C(save_restore_error, USBSTS, 10)
    BEK_BIT_ACCESSOR_RO(hc_not_ready, USBSTS, 11)
    BEK_BIT_ACCESSOR_RO(hc_error, USBSTS, 12)

    uSize page_size() const { return static_cast<u32>(PAGESIZE() & 0xFFFF) << 12; }

    bool notification_enable(u8 n) const { return (DNCTRL() >> n) & 1; }
    void notification_enable(u8 n, bool v) {
        DNCTRL(v ? BEK_SET_BIT(DNCTRL(), n) : BEK_CLEAR_BIT(DNCTRL(), n));
    }

    BEK_BIT_ACCESSOR_RW(ring_cycle_state, CRCR, 0)
    BEK_BIT_ACCESSOR_RW1S(command_stop, CRCR, 1)
    BEK_BIT_ACCESSOR_RW1S(command_abort, CRCR, 2)
    BEK_BIT_ACCESSOR_RO(command_ring_running, CRCR, 3)

    void set_command_ring_pointer(u64 ptr, bool ccs) {
        ASSERT((ptr & 0b111111) == 0);
        CRCR(CRCR() | ptr | (ccs ? 1 : 0));
    }

    u8 max_device_slots_enabled() const { return CONFIG() & 0xFF; }
    void max_device_slots_enabled(u8 v) { CONFIG((CONFIG() & ~0xFF) | v); }

    BEK_BIT_ACCESSOR_RW(u3_entry_enable, CONFIG, 8)
    BEK_BIT_ACCESSOR_RW(config_info_enable, CONFIG, 9)

    /// NOTE: n starts at 1
    PortOperationalRegisters port(u32 n) const {
        VERIFY(n >= 1);
        return PortOperationalRegisters{base.subdivide(0x400 + 0x10 * (n - 1), 0x10)};
    }

    mem::PCIeDeviceArea base;
};

struct InterrupterRegisters {
    BEK_REGISTER_P(IMAN, u32, 0x00, ~0b1)
    BEK_REGISTER(IMOD, u32, 0x04)
    BEK_REGISTER(ERSTSZ, u32, 0x08)
    BEK_REGISTER(ERSTBA, u64, 0x10)
    BEK_REGISTER(ERDP, u64, 0x18)

    BEK_BIT_ACCESSOR_RW1C(interrupt_pending, IMAN, 0)
    BEK_BIT_ACCESSOR_RW(interrupt_enable, IMAN, 1)

    void update_erdp(u64 dequeue_ptr, bool clear_busy_flag) {
        ERDP((dequeue_ptr & ~static_cast<u64>(0xF)) | ((clear_busy_flag ? 1 : 0) << 3));
    }

    mem::PCIeDeviceArea base;
};

struct RuntimeRegisters {
    constexpr static uSize BANK_SIZE = 0x8000;
    BEK_REGISTER_RO(MFINDEX, u32, 0)

    [[nodiscard]] InterrupterRegisters interrupter(u16 n) const {
        ASSERT(n < 1024);
        return {base.subdivide(0x20 + 32 * n, 32)};
    }

    mem::PCIeDeviceArea base;
};

struct DoorbellRegisters {
    constexpr static uSize BANK_SIZE = 256 * 4;
    void ring_doorbell(u8 doorbell, u8 target, u32 task_id) {
        base.write<u32>(4 * static_cast<u32>(doorbell), (task_id << 16) | target);
    }

    mem::PCIeDeviceArea base;
};

#endif  // BEKOS_XHCI_REGISTERS_H
