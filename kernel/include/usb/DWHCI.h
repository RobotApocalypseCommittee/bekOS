/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <library/assert.h>
#include <library/types.h>
#include <memory_locations.h>


#define BIT(nr) (1UL << (nr))
#define TESTMASK(val, msk) ((val & msk) ? 1 : 0)

template <uPtr P>
class DWReg {
public:
    ALWAYS_INLINE static u32 get() { return *reinterpret_cast<volatile u32 *>(P); }

    ALWAYS_INLINE static void set(u32 val) { *reinterpret_cast<volatile u32 *>(P) = val; }
};

class DynDWReg {
private:
    uPtr m_ptr;

public:
    explicit DynDWReg(uPtr ptr) : m_ptr(ptr){};

    ALWAYS_INLINE u32 get() const { return *reinterpret_cast<volatile u32 *>(m_ptr); }

    ALWAYS_INLINE void set(u32 val) const { *reinterpret_cast<volatile u32 *>(m_ptr) = val; }
};

namespace DWHCI {
const uPtr ARM_USB_BASE      = PERIPHERAL_BASE + 0x980000;
const uPtr ARM_USB_CORE_BASE = ARM_USB_BASE;
const uPtr ARM_USB_HOST_BASE = ARM_USB_BASE + 0x400;
const uPtr ARM_USB_POWER     = ARM_USB_BASE + 0xE00;

using POWER = DWReg<ARM_USB_POWER>;
const u32 POWER_SLEEP_CLOCK_GATING = BIT(5);
const u32 POWER_STOP_PCLK = BIT(0);
}

namespace DWHCI::CORE {

using OTG_CTRL                    = DWReg<ARM_USB_CORE_BASE + 0x000>;
const u32 OTG_CTRL_HST_SET_HNP_EN = BIT(10);

using OTG_INT = DWReg<ARM_USB_CORE_BASE + 0x004>;

using AHB_CFG                          = DWReg<ARM_USB_CORE_BASE + 0x008>;
const u32 AHB_CFG_GLOBALINT_MASK       = BIT(0);
const u32 AHB_CFG_MAX_AXI_BURST__SHIFT = 1;         // BCM2835 only
const u32 AHB_CFG_MAX_AXI_BURST__MASK  = (3 << 1);  // BCM2835 only
const u32 AHB_CFG_WAIT_AXI_WRITES      = BIT(4);    // BCM2835 only
const u32 AHB_CFG_DMAENABLE            = BIT(5);
const u32 AHB_CFG_AHB_SINGLE           = BIT(23);

using USB_CFG                       = DWReg<ARM_USB_CORE_BASE + 0x00C>;
const u32 USB_CFG_PHYIF             = BIT(3);
const u32 USB_CFG_ULPI_UTMI_SEL     = BIT(4);
const u32 USB_CFG_PHY_SEL_FS        = BIT(6);
const u32 USB_CFG_SRP_CAPABLE       = BIT(8);
const u32 USB_CFG_HNP_CAPABLE       = BIT(9);
const u32 USB_CFG_ULPI_FSLS         = BIT(17);
const u32 USB_CFG_ULPI_CLK_SUS_M    = BIT(19);
const u32 USB_CFG_ULPI_EXT_VBUS_DRV = BIT(20);
const u32 USB_CFG_TERM_SEL_DL_PULSE = BIT(22);

using RESET                        = DWReg<ARM_USB_CORE_BASE + 0x010>;
const u32 RESET_SOFT_RESET         = BIT(0);
const u32 RESET_RX_FIFO_FLUSH      = BIT(4);
const u32 RESET_TX_FIFO_FLUSH      = BIT(5);
const u32 RESET_TX_FIFO_NUM__SHIFT = 6;
const u32 RESET_TX_FIFO_NUM__MASK  = (0x1F << 6);
const u32 RESET_AHB_IDLE           = BIT(31);

using INT_STAT               = DWReg<ARM_USB_CORE_BASE + 0x014>;
const u32 INT_STAT_SOF_INTR  = BIT(3);
const u32 INT_STAT_PORT_INTR = BIT(24);
const u32 INT_STAT_HC_INTR   = BIT(25);

using INT_MASK                     = DWReg<ARM_USB_CORE_BASE + 0x018>;
const u32 INT_MASK_MODE_MISMATCH   = BIT(1);
const u32 INT_MASK_SOF_INTR        = BIT(3);
const u32 INT_MASK_RX_STS_Q_LVL    = BIT(4);
const u32 INT_MASK_USB_SUSPEND     = BIT(11);
const u32 INT_MASK_PORT_INTR       = BIT(24);
const u32 INT_MASK_HC_INTR         = BIT(25);
const u32 INT_MASK_CON_ID_STS_CHNG = BIT(28);
const u32 INT_MASK_DISCONNECT      = BIT(29);
const u32 INT_MASK_SESS_REQ_INTR   = BIT(30);
const u32 INT_MASK_WKUP_INTR       = BIT(31);

using RX_STAT_RD  = DWReg<ARM_USB_CORE_BASE + 0x01C>;
using RX_STAT_POP = DWReg<ARM_USB_CORE_BASE + 0x020>;
// for read and pop register in host mode
const u32 RX_STAT_CHAN_NUMBER__MASK             = 0xF;
const u32 RX_STAT_BYTE_COUNT__SHIFT             = 4;
const u32 RX_STAT_BYTE_COUNT__MASK              = (0x7FF << 4);
const u32 RX_STAT_PACKET_STATUS__SHIFT          = 17;
const u32 RX_STAT_PACKET_STATUS__MASK           = (0xF << 17);
const u32 RX_STAT_PACKET_STATUS_IN              = 2;
const u32 RX_STAT_PACKET_STATUS_IN_XFER_COMP    = 3;
const u32 RX_STAT_PACKET_STATUS_DATA_TOGGLE_ERR = 5;
const u32 RX_STAT_PACKET_STATUS_CHAN_HALTED     = 7;
using RX_FIFO_SIZ                               = DWReg<ARM_USB_CORE_BASE + 0x024>;
using NPER_TX_FIFO_SIZ                          = DWReg<ARM_USB_CORE_BASE + 0x028>;
using NPER_TX_STAT                              = DWReg<ARM_USB_CORE_BASE + 0x02C>;  // RO
constexpr u32 NPER_TX_STAT_QUEUE_SPACE_AVL(u32 reg) { return (reg >> 16) & 0xFF; }

using I2C_CTRL        = DWReg<ARM_USB_CORE_BASE + 0x030>;
using PHY_VENDOR_CTRL = DWReg<ARM_USB_CORE_BASE + 0x034>;
using GPIO            = DWReg<ARM_USB_CORE_BASE + 0x038>;
using USER_ID         = DWReg<ARM_USB_CORE_BASE + 0x03C>;
using VENDOR_ID       = DWReg<ARM_USB_CORE_BASE + 0x040>;
using HW_CFG1         = DWReg<ARM_USB_CORE_BASE + 0x044>;  // RO
using HW_CFG2         = DWReg<ARM_USB_CORE_BASE + 0x048>;  // RO

enum HW_CFG2_OP_MODES {
    HNP_SRP_CAPABLE,
    SRP_ONLY_CAPABLE,
    NO_HNP_SRP_CAPABLE,
    SRP_CAPABLE_DEVICE,
    NO_SRP_CAPABLE_DEVICE,
    SRP_CAPABLE_HOST,
    NO_SRP_CAPABLE_HOST,
};
constexpr HW_CFG2_OP_MODES HW_CFG2_OP_MODE(u32 reg) { return static_cast<HW_CFG2_OP_MODES>(((reg) >> 0) & 7); }



constexpr u32 HW_CFG2_ARCHITECTURE(u32 reg) { return ((reg) >> 3) & 3; }

constexpr u32 HW_CFG2_HS_PHY_TYPE(u32 reg) { return ((reg) >> 6) & 3; }

const u32 HW_CFG2_HS_PHY_TYPE_NOT_SUPPORTED = 0;
const u32 HW_CFG2_HS_PHY_TYPE_UTMI          = 1;
const u32 HW_CFG2_HS_PHY_TYPE_ULPI          = 2;
const u32 HW_CFG2_HS_PHY_TYPE_UTMI_ULPI     = 3;

constexpr u32 HW_CFG2_FS_PHY_TYPE(u32 reg) { return ((reg) >> 8) & 3; }

const u32 HW_CFG2_FS_PHY_TYPE_DEDICATED = 1;

constexpr u32 HW_CFG2_NUM_HOST_CHANNELS(u32 reg) { return (((reg) >> 14) & 0xF) + 1; }
const u32 MAX_HOST_CHANNELS = 16;

using HW_CFG3 = DWReg<ARM_USB_CORE_BASE + 0x04C>;  // RO
constexpr u32 HW_CFG3_DFIFO_DEPTH(u32 reg) { return ((reg) >> 16) & 0xFFFF; }

using HW_CFG4                 = DWReg<ARM_USB_CORE_BASE + 0x050>;  // RO
const u32 HW_CFG4_DED_FIFO_EN = BIT(25);

constexpr u32 HW_CFG4_NUM_IN_EPS(u32 reg) { return ((reg) >> 26) & 0xF; }

using LPM_CFG                          = DWReg<ARM_USB_CORE_BASE + 0x054>;
using POWER_DOWN                       = DWReg<ARM_USB_CORE_BASE + 0x058>;
using DFIFO_CFG                        = DWReg<ARM_USB_CORE_BASE + 0x05C>;
const u32 DFIFO_CFG_EPINFO_BASE__SHIFT = 16;
const u32 DFIFO_CFG_EPINFO_BASE__MASK  = (0xFFFF << 16);
using ADP_CTRL                         = DWReg<ARM_USB_CORE_BASE + 0x060>;
}

namespace DWHCI::VENDOR {
// gap
using MDIO_CTRL = DWReg<ARM_USB_CORE_BASE + 0x080>;  // BCM2835 only
using MDIO_DATA = DWReg<ARM_USB_CORE_BASE + 0x084>;  // BCM2835 only
using VBUS_DRV  = DWReg<ARM_USB_CORE_BASE + 0x088>;  // BCM2835 only
};

namespace DWHCI::CORE {
// gap
using HOST_PER_TX_FIFO_SIZ = DWReg<ARM_USB_CORE_BASE + 0x100>;
// fifo := 0..14 :
template <uSize fifo>
using DEV_PER_TX_FIFO = DWReg<ARM_USB_CORE_BASE + 0x104 + (fifo)*4>;  // dedicated FIFOs on
template <uSize fifo>
using DEV_TX_FIFO = DWReg<ARM_USB_CORE_BASE + 0x104 + (fifo)*4>;  // dedicated FIFOs off
};

namespace DWHCI::HOST {
//
// Host Registers
//
using CFG                             = DWReg<ARM_USB_HOST_BASE + 0x000>;
const u32 CFG_FSLS_PCLK_SEL__SHIFT    = 0;
const u32 CFG_FSLS_PCLK_SEL__MASK     = (3 << 0);
const u32 CFG_FSLS_PCLK_SEL_30_60_MHZ = 0;
const u32 CFG_FSLS_PCLK_SEL_48_MHZ    = 1;
const u32 CFG_FSLS_PCLK_SEL_6_MHZ     = 2;
const u32 CFG_FSLS_ONLY = BIT(2);
const u32 CFG_EN_DMA_DESC = BIT(23);
using FRM_INTERVAL                    = DWReg<ARM_USB_HOST_BASE + 0x004>;
using FRM_NUM                         = DWReg<ARM_USB_HOST_BASE + 0x008>;

constexpr u32 FRM_NUM_NUMBER(u32 reg) { return reg & 0xFFFF; }

#define DWHCI_MAX_FRAME_NUMBER 0x3FFF

constexpr u32 FRM_NUM_REMAINING(u32 reg) { return (reg >> 16) & 0xFFFF; }
// gap
using PER_TX_FIFO_STAT = DWReg<ARM_USB_HOST_BASE + 0x010>;
using ALLCHAN_INT      = DWReg<ARM_USB_HOST_BASE + 0x014>;
using ALLCHAN_INT_MASK = DWReg<ARM_USB_HOST_BASE + 0x018>;
using FRMLST_BASE      = DWReg<ARM_USB_HOST_BASE + 0x01C>;
// gap
using PORT                         = DWReg<ARM_USB_HOST_BASE + 0x040>;
const u32 PORT_CONNECT             = BIT(0);
const u32 PORT_CONNECT_CHANGED     = BIT(1);
const u32 PORT_ENABLE              = BIT(2);
const u32 PORT_ENABLE_CHANGED      = BIT(3);
const u32 PORT_OVERCURRENT         = BIT(4);
const u32 PORT_OVERCURRENT_CHANGED = BIT(5);
const u32 PORT_RESUME = BIT(6);
const u32 PORT_SUSPEND = BIT(7);
const u32 PORT_RESET               = BIT(8);
const u32 PORT_POWER               = BIT(12);
constexpr u32 PORT_TESTMODE(u32 reg) {return (reg >> 13) & 7; }

constexpr u32 PORT_SPEED(u32 reg) { return (reg >> 17) & 3; }

const u32 PORT_SPEED_HIGH = 0;
const u32 PORT_SPEED_FULL = 1;
const u32 PORT_SPEED_LOW  = 2;
const u32 PORT_DEFAULT_MASK =
    PORT_CONNECT_CHANGED | PORT_ENABLE | PORT_ENABLE_CHANGED | PORT_OVERCURRENT_CHANGED;
// gap
// chan := 0..15 :

ALWAYS_INLINE DynDWReg CHAN_CHARACTER(u32 chan) {
    return DynDWReg(ARM_USB_HOST_BASE + 0x100 + chan * 0x20);
}

const u32 CHAN_CHARACTER_MAX_PKT_SIZ__MASK     = 0x7FF;
const u32 CHAN_CHARACTER_EP_NUMBER__SHIFT      = 11;
const u32 CHAN_CHARACTER_EP_NUMBER__MASK       = (0xF << 11);
const u32 CHAN_CHARACTER_EP_DIRECTION_IN       = BIT(15);
const u32 CHAN_CHARACTER_LOW_SPEED_DEVICE      = BIT(17);
const u32 CHAN_CHARACTER_EP_TYPE__SHIFT        = 18;
const u32 CHAN_CHARACTER_EP_TYPE__MASK         = (3 << 18);
const u32 CHAN_CHARACTER_EP_TYPE_CONTROL       = 0;
const u32 CHAN_CHARACTER_EP_TYPE_ISO           = 1;
const u32 CHAN_CHARACTER_EP_TYPE_BULK          = 2;
const u32 CHAN_CHARACTER_EP_TYPE_INTERRUPT     = 3;
const u32 CHAN_CHARACTER_MULTI_CNT__SHIFT      = 20;
const u32 CHAN_CHARACTER_MULTI_CNT__MASK       = (3 << 20);
const u32 CHAN_CHARACTER_DEVICE_ADDRESS__SHIFT = 22;
const u32 CHAN_CHARACTER_DEVICE_ADDRESS__MASK  = (0x7F << 22);
const u32 CHAN_CHARACTER_PER_ODD_FRAME         = BIT(29);
const u32 CHAN_CHARACTER_DISABLE               = BIT(30);
const u32 CHAN_CHARACTER_ENABLE                = BIT(31);

ALWAYS_INLINE DynDWReg CHAN_SPLIT_CTRL(u32 chan) {
    return DynDWReg(ARM_USB_HOST_BASE + 0x104 + chan * 0x20);
}

const u32 CHAN_SPLIT_CTRL_PORT_ADDRESS__MASK = 0x7F;
const u32 CHAN_SPLIT_CTRL_HUB_ADDRESS__SHIFT = 7;
const u32 CHAN_SPLIT_CTRL_HUB_ADDRESS__MASK  = (0x7F << 7);
const u32 CHAN_SPLIT_CTRL_XACT_POS__SHIFT    = 14;
const u32 CHAN_SPLIT_CTRL_XACT_POS__MASK     = (3 << 14);
const u32 CHAN_SPLIT_CTRL_ALL                = 3;
const u32 CHAN_SPLIT_CTRL_COMPLETE_SPLIT     = BIT(16);
const u32 CHAN_SPLIT_CTRL_SPLIT_ENABLE       = BIT(31);

ALWAYS_INLINE DynDWReg CHAN_INT(u32 chan) {
    return DynDWReg(ARM_USB_HOST_BASE + 0x108 + chan * 0x20);
}

const u32 CHAN_INT_XFER_COMPLETE     = BIT(0);
const u32 CHAN_INT_HALTED            = BIT(1);
const u32 CHAN_INT_AHB_ERROR         = BIT(2);
const u32 CHAN_INT_STALL             = BIT(3);
const u32 CHAN_INT_NAK               = BIT(4);
const u32 CHAN_INT_ACK               = BIT(5);
const u32 CHAN_INT_NYET              = BIT(6);
const u32 CHAN_INT_XACT_ERROR        = BIT(7);
const u32 CHAN_INT_BABBLE_ERROR      = BIT(8);
const u32 CHAN_INT_FRAME_OVERRUN     = BIT(9);
const u32 CHAN_INT_DATA_TOGGLE_ERROR = BIT(10);
const u32 CHAN_INT_ERROR_MASK        = CHAN_INT_AHB_ERROR | CHAN_INT_STALL | CHAN_INT_XACT_ERROR |
                                CHAN_INT_BABBLE_ERROR | CHAN_INT_FRAME_OVERRUN |
                                CHAN_INT_DATA_TOGGLE_ERROR;

ALWAYS_INLINE DynDWReg CHAN_INT_MASK(u32 chan) {
    assert(chan < 16);
    return DynDWReg(ARM_USB_HOST_BASE + 0x10C + chan * 0x20);
}

ALWAYS_INLINE DynDWReg CHAN_XFER_SIZ(u32 chan) {
    return DynDWReg(ARM_USB_HOST_BASE + 0x110 + chan * 0x20);
}

const u32 CHAN_XFER_SIZ_BYTES__MASK    = 0x7FFFF;
const u32 CHAN_XFER_SIZ_PACKETS__SHIFT = 19;
const u32 CHAN_XFER_SIZ_PACKETS__MASK  = (0x3FF << 19);

constexpr u32 CHAN_XFER_SIZ_PACKETS(u32 reg) { return ((reg) >> 19) & 0x3FF; }

const u32 CHAN_XFER_SIZ_PID__SHIFT = 29;
const u32 CHAN_XFER_SIZ_PID__MASK  = (3 << 29);

constexpr u32 CHAN_XFER_SIZ_PID(u32 reg) { return ((reg) >> 29) & 3; }

const u32 CHAN_XFER_SIZ_PID_DATA0 = 0;
const u32 CHAN_XFER_SIZ_PID_DATA1 = 2;
const u32 CHAN_XFER_SIZ_PID_DATA2 = 1;
const u32 CHAN_XFER_SIZ_PID_MDATA = 3;  // non-control transfer
const u32 CHAN_XFER_SIZ_PID_SETUP = 3;

ALWAYS_INLINE DynDWReg CHAN_DMA_ADDR(u32 chan) {
    return DynDWReg(ARM_USB_HOST_BASE + 0x114 + chan * 0x20);
}
// gap
ALWAYS_INLINE DynDWReg CHAN_DMA_BUF(u32 chan) {
    return DynDWReg(ARM_USB_HOST_BASE + 0x11C + chan * 0x20);
}
}

//
// Data FIFOs (non-DMA mode only)
//
#define DWHCI_DATA_FIFO(chan) (ARM_USB_HOST_BASE + 0x1000 + (chan)*DWHCI_DATA_FIFO_SIZE)