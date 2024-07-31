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

#include "usb/DWHost.h"

#include <library/function.h>
#include <peripherals/property_tags.h>

#include "printf.h"
#include "usb/DWHCI.h"
using namespace DWHCI;

const bool usb_fullspeed    = false;
const bool usb_sof_intr     = false;
const u32 RX_FIFO_SIZE      = 1024;
const u32 NPER_TX_FIFO_SIZE = 1024;
const u32 PER_TX_FIFO_SIZE  = 1024;

extern interrupt_controller interruptController;

DWHost::DWHost() : m_root_port(this) {}

bool DWHost::init() {
    if (CORE::VENDOR_ID::get() != 0x4F54280A) {
        printf("Unknown Vendor");
        return false;
    }

    property_tags tags;
    if (!set_peripheral_power_state(tags, BCMDevices::USB, true)) {
        printf("Cannot turn on via BCM");
        return false;
    }

    // First disable interrupts
    auto ahb_config = CORE::AHB_CFG::get();
    ahb_config &= ~CORE::AHB_CFG_GLOBALINT_MASK;
    CORE::AHB_CFG::set(ahb_config);

    // Connect interrupt
    interruptController.register_handler(interrupt_controller::USB,
                                         [this]() { return handle_interrupts(); });
    interruptController.enable(interrupt_controller::USB);

    if (!init_core()) {
        printf("Failed to initialize core");
        return false;
    }

    // Enable global interrupts
    ahb_config = CORE::AHB_CFG::get();
    ahb_config |= CORE::AHB_CFG_GLOBALINT_MASK;
    CORE::AHB_CFG::set(ahb_config);

    if (!init_host()) {
        printf("Failed to initialize host");
        return false;
    }

    rescan_devices();

    return true;
}

bool DWHost::rescan_devices() {
    if (m_root_port_initialised) {
        return m_root_port.rescan_devices();
    } else {
        if (!enable_root_port()) {
            return false;
        }
        return m_root_port.init();
    }
}

PortSpeed DWHost::get_port_speed() {
    auto raw_speed = HOST::PORT_SPEED(HOST::PORT::get());
    switch (raw_speed) {
        case HOST::PORT_SPEED_HIGH:
            return PortSpeed::High;
        case HOST::PORT_SPEED_LOW:
            return PortSpeed::Low;
        case HOST::PORT_SPEED_FULL:
            return PortSpeed::Full;
        default:
            return PortSpeed::Unknown;
    }
}

bool DWHost::is_device_connected() { return HOST::PORT::get() & HOST::PORT_CONNECT; }

bool DWHost::init_core() {
    auto usb_config = CORE::USB_CFG::get();
    if constexpr (usb_fullspeed) {
        usb_config |= CORE::USB_CFG_PHY_SEL_FS;
    }
    usb_config &= ~CORE::USB_CFG_ULPI_EXT_VBUS_DRV;
    usb_config &= ~CORE::USB_CFG_TERM_SEL_DL_PULSE;
    CORE::USB_CFG::set(usb_config);

    if (!reset()) {
        printf("Could not reset");
        return false;
    }

    usb_config = CORE::USB_CFG::get();
    usb_config &= CORE::USB_CFG_ULPI_UTMI_SEL;
    usb_config &= CORE::USB_CFG_PHYIF;
    CORE::USB_CFG::set(usb_config);

    auto hw_config2 = CORE::HW_CFG2::get();
    // Check we use internal DMA
    ASSERT(CORE::HW_CFG2_ARCHITECTURE(hw_config2) == 2);

    usb_config = CORE::USB_CFG::get();
    if (CORE::HW_CFG2_HS_PHY_TYPE(hw_config2) == CORE::HW_CFG2_HS_PHY_TYPE_ULPI &&
        CORE::HW_CFG2_FS_PHY_TYPE(hw_config2) == CORE::HW_CFG2_FS_PHY_TYPE_DEDICATED) {
        usb_config |= CORE::USB_CFG_ULPI_FSLS;
        usb_config |= CORE::USB_CFG_ULPI_CLK_SUS_M;
    } else {
        usb_config &= ~CORE::USB_CFG_ULPI_FSLS;
        usb_config &= ~CORE::USB_CFG_ULPI_CLK_SUS_M;
    }
    CORE::USB_CFG::set(usb_config);

    m_channel_count = CORE::HW_CFG2_NUM_HOST_CHANNELS(hw_config2);
    ASSERT(4 <= m_channel_count && m_channel_count < CORE::MAX_HOST_CHANNELS);

    auto ahb_config = CORE::AHB_CFG::get();
    ahb_config |= CORE::AHB_CFG_DMAENABLE;
    ahb_config |= CORE::AHB_CFG_WAIT_AXI_WRITES;
    ahb_config &= ~CORE::AHB_CFG_MAX_AXI_BURST__MASK;
    CORE::AHB_CFG::set(ahb_config);

    usb_config = CORE::USB_CFG::get();
    usb_config &= ~CORE::USB_CFG_HNP_CAPABLE;
    usb_config &= ~CORE::USB_CFG_SRP_CAPABLE;
    CORE::USB_CFG::set(usb_config);

    // Clear all pending interrupts
    CORE::INT_STAT::set(-1);

    return true;
}
bool DWHost::init_host() {
    POWER::set(0);

    auto host_config = HOST::CFG::get();
    host_config &= ~HOST::CFG_FSLS_PCLK_SEL__MASK;

    u32 hw_config2 = CORE::HW_CFG2::get();
    if (CORE::HW_CFG2_HS_PHY_TYPE(hw_config2) == CORE::HW_CFG2_HS_PHY_TYPE_ULPI &&
        CORE::HW_CFG2_FS_PHY_TYPE(hw_config2) == CORE::HW_CFG2_FS_PHY_TYPE_DEDICATED &&
        (CORE::USB_CFG::get() & CORE::USB_CFG_ULPI_FSLS)) {
        host_config |= HOST::CFG_FSLS_PCLK_SEL_48_MHZ;
    } else {
        host_config |= HOST::CFG_FSLS_PCLK_SEL_30_60_MHZ;
    }
    HOST::CFG::set(host_config);

    CORE::RX_FIFO_SIZ::set(RX_FIFO_SIZE);
    CORE::NPER_TX_FIFO_SIZ::set(0 | RX_FIFO_SIZE | NPER_TX_FIFO_SIZE << 16);
    CORE::HOST_PER_TX_FIFO_SIZ::set(0 | (RX_FIFO_SIZE + NPER_TX_FIFO_SIZE) |
                                    PER_TX_FIFO_SIZE << 16);

    flush_tx_fifo(0x10);
    flush_rx_fifo();

    auto host_port = HOST::PORT::get();
    host_port &= ~HOST::PORT_DEFAULT_MASK;
    if (!(host_port & HOST::PORT_POWER)) {
        host_port |= HOST::PORT_POWER;
        HOST::PORT::set(host_port);
    }

    enable_host_interrupts();

    return true;
}
bool DWHost::reset() {
    // Wait for idle
    while (!(CORE::RESET::get() & CORE::RESET_AHB_IDLE)) {
        // TODO: Timeout 100ms
    }
    CORE::RESET::set(CORE::RESET::get() | CORE::RESET_SOFT_RESET);

    while (CORE::RESET::get() & CORE::RESET_SOFT_RESET) {
        // TODO: Timeout 10ms
    }

    // TODO: Wait 100ms

    return true;
}
void DWHost::enable_host_interrupts() {
    // Disable all
    CORE::INT_MASK::set(0);

    // Clear pending interrupts
    CORE::INT_STAT::set(-1);

    auto mask = CORE::INT_MASK::get();
    mask |= CORE::INT_MASK_HC_INTR;
    // TODO: Maybe?
    if constexpr (usb_sof_intr) {
        mask |= CORE::INT_MASK_SOF_INTR;
    }
    // TODO: Is plug and play?
    mask |= CORE::INT_MASK_PORT_INTR;
    mask |= CORE::INT_MASK_DISCONNECT;

    CORE::INT_MASK::set(mask);
}
void DWHost::flush_tx_fifo(u32 n_fifo) {
    auto reset = (CORE::RESET_TX_FIFO_FLUSH & ~CORE::RESET_TX_FIFO_NUM__MASK) |
                 n_fifo << CORE::RESET_TX_FIFO_NUM__SHIFT;
    CORE::RESET::set(reset);

    while (CORE::RESET::get() & CORE::RESET_TX_FIFO_FLUSH) {
        // TODO: Timeout 10ms
    }
    // TODO: Wait 1uS
}
void DWHost::flush_rx_fifo() {
    CORE::RESET::set(CORE::RESET_RX_FIFO_FLUSH);

    while (CORE::RESET::get() & CORE::RESET_RX_FIFO_FLUSH) {
        // TODO: TImeout 10ms
    }
    // TODO: Wait 1uS
}
bool DWHost::enable_root_port() {
    const u32 delay_ms = 510;  // Standard
    while (!(HOST::PORT::get() & HOST::PORT_CONNECT)) {
        // TODO: Timeout delay_ms ms
    }
    // TODO: Delay 100ms
    auto host_port = HOST::PORT::get();
    host_port &= ~HOST::PORT_DEFAULT_MASK;
    host_port |= HOST::PORT_RESET;
    HOST::PORT::set(host_port);

    // TODO: Delay 50ms

    host_port = HOST::PORT::get();
    host_port &= ~HOST::PORT_DEFAULT_MASK;
    host_port &= ~HOST::PORT_RESET;
    HOST::PORT::set(host_port);

    // TODO: Delay 10ms/20ms

    return true;
}
