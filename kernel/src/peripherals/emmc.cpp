/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Modified for use in bekOS by J. Bell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Provides an interface to the EMMC controller and commands for interacting
 * with an sd card */

/* References:
 *
 * PLSS 	- SD Group Physical Layer Simplified Specification ver 3.00
 * HCSS		- SD Group Host Controller Simplified Specification ver 3.00
 *
 * Broadcom BCM2835 Peripherals Guide
 */

#include "peripherals/emmc.h"

#include <kstring.h>
#include <library/utility.h>
#include <utils.h>

#include "peripherals/gentimer.h"
#include "peripherals/gpio.h"
#include "peripherals/property_tags.h"
#include "printf.h"

//#define DEBUG2

#ifdef DEBUG2
#define EMMC_DEBUG
#endif

// Configuration options

// Enable 1.8V support
#define SD_1_8V_SUPPORT

// Enable 4-bit support
#define SD_4BIT_DATA

// SD Clock Frequencies (in Hz)
#define SD_CLOCK_ID 400000
#define SD_CLOCK_NORMAL 25000000
#define SD_CLOCK_HIGH 50000000
#define SD_CLOCK_100 100000000
#define SD_CLOCK_208 208000000

// Enable SDXC maximum performance mode
// Requires 150 mA power so disabled on the RPi for now
//#define SDXC_MAXIMUM_PERFORMANCE

// Enable SDMA support
//#define SDMA_SUPPORT

// SDMA buffer address
#define SDMA_BUFFER 0x6000
#define SDMA_BUFFER_PA (SDMA_BUFFER + 0xC0000000)

// Enable card interrupts
//#define SD_CARD_INTERRUPTS

// Enable EXPERIMENTAL (and possibly DANGEROUS) SD write support
#define SD_WRITE_SUPPORT

// Allow old sdhci versions (may cause errors)
#define EMMC_ALLOW_OLD_SDHCI

#define EMMC_BASE PERIPHERAL_BASE + 0x300000
#define EMMC_ARG2 0
#define EMMC_BLKSIZECNT 4
#define EMMC_ARG1 8
#define EMMC_CMDTM 0xC
#define EMMC_RESP0 0x10
#define EMMC_RESP1 0x14
#define EMMC_RESP2 0x18
#define EMMC_RESP3 0x1C
#define EMMC_DATA 0x20
#define EMMC_STATUS 0x24
#define EMMC_CONTROL0 0x28
#define EMMC_CONTROL1 0x2C
#define EMMC_INTERRUPT 0x30
#define EMMC_IRPT_MASK 0x34
#define EMMC_IRPT_EN 0x38
#define EMMC_CONTROL2 0x3C
#define EMMC_CAPABILITIES_0 0x40
#define EMMC_CAPABILITIES_1 0x44
#define EMMC_FORCE_IRPT 0x50
#define EMMC_BOOT_TIMEOUT 0x70
#define EMMC_DBG_SEL 0x74
#define EMMC_EXRDFIFO_CFG 0x80
#define EMMC_EXRDFIFO_EN 0x84
#define EMMC_TUNE_STEP 0x88
#define EMMC_TUNE_STEPS_STD 0x8C
#define EMMC_TUNE_STEPS_DDR 0x90
#define EMMC_SPI_INT_SPT 0xF0
#define EMMC_SLOTISR_VER 0xFC

#define SD_CMD_INDEX(a) ((a) << 24)
#define SD_CMD_TYPE_NORMAL 0x0
#define SD_CMD_TYPE_SUSPEND (1 << 22)
#define SD_CMD_TYPE_RESUME (2 << 22)
#define SD_CMD_TYPE_ABORT (3 << 22)
#define SD_CMD_TYPE_MASK (3 << 22)
#define SD_CMD_ISDATA (1 << 21)
#define SD_CMD_IXCHK_EN (1 << 20)
#define SD_CMD_CRCCHK_EN (1 << 19)
#define SD_CMD_RSPNS_TYPE_NONE 0         // For no response
#define SD_CMD_RSPNS_TYPE_136 (1 << 16)  // For response R2 (with CRC), R3,4 (no CRC)
#define SD_CMD_RSPNS_TYPE_48 (2 << 16)   // For responses R1, R5, R6, R7 (with CRC)
#define SD_CMD_RSPNS_TYPE_48B (3 << 16)  // For responses R1b, R5b (with CRC)
#define SD_CMD_RSPNS_TYPE_MASK (3 << 16)
#define SD_CMD_MULTI_BLOCK (1 << 5)
#define SD_CMD_DAT_DIR_HC 0
#define SD_CMD_DAT_DIR_CH (1 << 4)
#define SD_CMD_AUTO_CMD_EN_NONE 0
#define SD_CMD_AUTO_CMD_EN_CMD12 (1 << 2)
#define SD_CMD_AUTO_CMD_EN_CMD23 (2 << 2)
#define SD_CMD_BLKCNT_EN (1 << 1)
#define SD_CMD_DMA 1

#define SD_ERR_CMD_TIMEOUT 0
#define SD_ERR_CMD_CRC 1
#define SD_ERR_CMD_END_BIT 2
#define SD_ERR_CMD_INDEX 3
#define SD_ERR_DATA_TIMEOUT 4
#define SD_ERR_DATA_CRC 5
#define SD_ERR_DATA_END_BIT 6
#define SD_ERR_CURRENT_LIMIT 7
#define SD_ERR_AUTO_CMD12 8
#define SD_ERR_ADMA 9
#define SD_ERR_TUNING 10
#define SD_ERR_RSVD 11

#define SD_ERR_MASK_CMD_TIMEOUT (1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_CMD_CRC (1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_CMD_END_BIT (1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CMD_INDEX (1 << (16 + SD_ERR_CMD_INDEX))
#define SD_ERR_MASK_DATA_TIMEOUT (1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_DATA_CRC (1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_DATA_END_BIT (1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CURRENT_LIMIT (1 << (16 + SD_ERR_CMD_CURRENT_LIMIT))
#define SD_ERR_MASK_AUTO_CMD12 (1 << (16 + SD_ERR_CMD_AUTO_CMD12))
#define SD_ERR_MASK_ADMA (1 << (16 + SD_ERR_CMD_ADMA))
#define SD_ERR_MASK_TUNING (1 << (16 + SD_ERR_CMD_TUNING))

#define SD_COMMAND_COMPLETE 1
#define SD_TRANSFER_COMPLETE (1 << 1)
#define SD_BLOCK_GAP_EVENT (1 << 2)
#define SD_DMA_INTERRUPT (1 << 3)
#define SD_BUFFER_WRITE_READY (1 << 4)
#define SD_BUFFER_READ_READY (1 << 5)
#define SD_CARD_INSERTION (1 << 6)
#define SD_CARD_REMOVAL (1 << 7)
#define SD_CARD_INTERRUPT (1 << 8)

#define SD_RESP_NONE SD_CMD_RSPNS_TYPE_NONE
#define SD_RESP_R1 (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R1b (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R2 (SD_CMD_RSPNS_TYPE_136 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R3 SD_CMD_RSPNS_TYPE_48
#define SD_RESP_R4 SD_CMD_RSPNS_TYPE_136
#define SD_RESP_R5 (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R5b (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R6 (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R7 (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)

#define SD_DATA_READ (SD_CMD_ISDATA | SD_CMD_DAT_DIR_CH)
#define SD_DATA_WRITE (SD_CMD_ISDATA | SD_CMD_DAT_DIR_HC)

#define SD_CMD_RESERVED(a) 0xffffffff

#define SUCCESS (last_cmd_success)
#define FAIL (last_cmd_success == 0)
#define TIMEOUT (FAIL && (last_error == 0))
#define CMD_TIMEOUT (FAIL && (last_error & (1 << 16)))
#define CMD_CRC (FAIL && (last_error & (1 << 17)))
#define CMD_END_BIT (FAIL && (last_error & (1 << 18)))
#define CMD_INDEX (FAIL && (last_error & (1 << 19)))
#define DATA_TIMEOUT (FAIL && (last_error & (1 << 20)))
#define DATA_CRC (FAIL && (last_error & (1 << 21)))
#define DATA_END_BIT (FAIL && (last_error & (1 << 22)))
#define CURRENT_LIMIT (FAIL && (last_error & (1 << 23)))
#define ACMD12_ERROR (FAIL && (last_error & (1 << 24)))
#define ADMA_ERROR (FAIL && (last_error & (1 << 25)))
#define TUNING_ERROR (FAIL && (last_error & (1 << 26)))

#define SD_VER_UNKNOWN 0
#define SD_VER_1 1
#define SD_VER_1_1 2
#define SD_VER_2 3
#define SD_VER_3 4
#define SD_VER_4 5

const char *SDCard::sd_versions[] = {"unknown", "1.0 and 1.01", "1.10", "2.00", "3.0x", "4.xx"};

#ifdef EMMC_DEBUG
const char *SDCard::err_irpts[] = {"CMD_TIMEOUT",  "CMD_CRC",  "CMD_END_BIT",  "CMD_INDEX",
                                   "DATA_TIMEOUT", "DATA_CRC", "DATA_END_BIT", "CURRENT_LIMIT",
                                   "AUTO_CMD12",   "ADMA",     "TUNING",       "RSVD"};
#endif

u32 hci_ver;

u32 capabilities_0;
u32 capabilities_1;
const u32 SDCard::sd_commands[] = {
    SD_CMD_INDEX(0),
    SD_CMD_RESERVED(1),
    SD_CMD_INDEX(2) | SD_RESP_R2,
    SD_CMD_INDEX(3) | SD_RESP_R6,
    SD_CMD_INDEX(4),
    SD_CMD_INDEX(5) | SD_RESP_R4,
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_INDEX(7) | SD_RESP_R1b,
    SD_CMD_INDEX(8) | SD_RESP_R7,
    SD_CMD_INDEX(9) | SD_RESP_R2,
    SD_CMD_INDEX(10) | SD_RESP_R2,
    SD_CMD_INDEX(11) | SD_RESP_R1,
    SD_CMD_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT,
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_INDEX(15),
    SD_CMD_INDEX(16) | SD_RESP_R1,
    SD_CMD_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(18) | SD_RESP_R1 | SD_DATA_READ | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(20) | SD_RESP_R1b,
    SD_CMD_RESERVED(21),
    SD_CMD_RESERVED(22),
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_RESERVED(26),
    SD_CMD_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(28) | SD_RESP_R1b,
    SD_CMD_INDEX(29) | SD_RESP_R1b,
    SD_CMD_INDEX(30) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(31),
    SD_CMD_INDEX(32) | SD_RESP_R1,
    SD_CMD_INDEX(33) | SD_RESP_R1,
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_INDEX(38) | SD_RESP_R1b,
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_RESERVED(41),
    SD_CMD_RESERVED(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_RESERVED(51),
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_INDEX(55) | SD_RESP_R1,
    SD_CMD_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA,
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)};

const u32 SDCard::sd_acommands[] = {SD_CMD_RESERVED(0),
                                    SD_CMD_RESERVED(1),
                                    SD_CMD_RESERVED(2),
                                    SD_CMD_RESERVED(3),
                                    SD_CMD_RESERVED(4),
                                    SD_CMD_RESERVED(5),
                                    SD_CMD_INDEX(6) | SD_RESP_R1,
                                    SD_CMD_RESERVED(7),
                                    SD_CMD_RESERVED(8),
                                    SD_CMD_RESERVED(9),
                                    SD_CMD_RESERVED(10),
                                    SD_CMD_RESERVED(11),
                                    SD_CMD_RESERVED(12),
                                    SD_CMD_INDEX(13) | SD_RESP_R1,
                                    SD_CMD_RESERVED(14),
                                    SD_CMD_RESERVED(15),
                                    SD_CMD_RESERVED(16),
                                    SD_CMD_RESERVED(17),
                                    SD_CMD_RESERVED(18),
                                    SD_CMD_RESERVED(19),
                                    SD_CMD_RESERVED(20),
                                    SD_CMD_RESERVED(21),
                                    SD_CMD_INDEX(22) | SD_RESP_R1 | SD_DATA_READ,
                                    SD_CMD_INDEX(23) | SD_RESP_R1,
                                    SD_CMD_RESERVED(24),
                                    SD_CMD_RESERVED(25),
                                    SD_CMD_RESERVED(26),
                                    SD_CMD_RESERVED(27),
                                    SD_CMD_RESERVED(28),
                                    SD_CMD_RESERVED(29),
                                    SD_CMD_RESERVED(30),
                                    SD_CMD_RESERVED(31),
                                    SD_CMD_RESERVED(32),
                                    SD_CMD_RESERVED(33),
                                    SD_CMD_RESERVED(34),
                                    SD_CMD_RESERVED(35),
                                    SD_CMD_RESERVED(36),
                                    SD_CMD_RESERVED(37),
                                    SD_CMD_RESERVED(38),
                                    SD_CMD_RESERVED(39),
                                    SD_CMD_RESERVED(40),
                                    SD_CMD_INDEX(41) | SD_RESP_R3,
                                    SD_CMD_INDEX(42) | SD_RESP_R1,
                                    SD_CMD_RESERVED(43),
                                    SD_CMD_RESERVED(44),
                                    SD_CMD_RESERVED(45),
                                    SD_CMD_RESERVED(46),
                                    SD_CMD_RESERVED(47),
                                    SD_CMD_RESERVED(48),
                                    SD_CMD_RESERVED(49),
                                    SD_CMD_RESERVED(50),
                                    SD_CMD_INDEX(51) | SD_RESP_R1 | SD_DATA_READ,
                                    SD_CMD_RESERVED(52),
                                    SD_CMD_RESERVED(53),
                                    SD_CMD_RESERVED(54),
                                    SD_CMD_RESERVED(55),
                                    SD_CMD_RESERVED(56),
                                    SD_CMD_RESERVED(57),
                                    SD_CMD_RESERVED(58),
                                    SD_CMD_RESERVED(59),
                                    SD_CMD_RESERVED(60),
                                    SD_CMD_RESERVED(61),
                                    SD_CMD_RESERVED(62),
                                    SD_CMD_RESERVED(63)};

// The actual command indices
#define GO_IDLE_STATE 0
#define ALL_SEND_CID 2
#define SEND_RELATIVE_ADDR 3
#define SET_DSR 4
#define IO_SET_OP_COND 5
#define SWITCH_FUNC 6
#define SELECT_CARD 7
#define DESELECT_CARD 7
#define SELECT_DESELECT_CARD 7
#define SEND_IF_COND 8
#define SEND_CSD 9
#define SEND_CID 10
#define VOLTAGE_SWITCH 11
#define STOP_TRANSMISSION 12
#define SEND_STATUS 13
#define GO_INACTIVE_STATE 15
#define SET_BLOCKLEN 16
#define READ_SINGLE_BLOCK 17
#define READ_MULTIPLE_BLOCK 18
#define SEND_TUNING_BLOCK 19
#define SPEED_CLASS_CONTROL 20
#define SET_BLOCK_COUNT 23
#define WRITE_BLOCK 24
#define WRITE_MULTIPLE_BLOCK 25
#define PROGRAM_CSD 27
#define SET_WRITE_PROT 28
#define CLR_WRITE_PROT 29
#define SEND_WRITE_PROT 30
#define ERASE_WR_BLK_START 32
#define ERASE_WR_BLK_END 33
#define ERASE 38
#define LOCK_UNLOCK 42
#define APP_CMD 55
#define GEN_CMD 56

#define IS_APP_CMD 0x80000000
#define ACMD(a) (a | IS_APP_CMD)
#define SET_BUS_WIDTH (6 | IS_APP_CMD)
#define SD_STATUS (13 | IS_APP_CMD)
#define SEND_NUM_WR_BLOCKS (22 | IS_APP_CMD)
#define SET_WR_BLK_ERASE_COUNT (23 | IS_APP_CMD)
#define SD_SEND_OP_COND (41 | IS_APP_CMD)
#define SET_CLR_CARD_DETECT (42 | IS_APP_CMD)
#define SEND_SCR (51 | IS_APP_CMD)

#define SD_RESET_CMD (1 << 25)
#define SD_RESET_DAT (1 << 26)
#define SD_RESET_ALL (1 << 24)

#define SD_GET_CLOCK_DIVIDER_FAIL 0xffffffff

bool timeout_wait(u64 read_address, u32 mask, unsigned int value, unsigned int usec_timeout) {
    do {
        if ((mmio_read(read_address) & mask) ? value : !value) {
            return true;
        }
        // TODO: Fix
        bad_udelay(1);
    } while (usec_timeout--);
    return false;
}

bool SDCard::mbox_power_on() {
    property_tags tags;
    return set_peripheral_power_state(tags, BCMDevices::SD, true);
}

bool SDCard::mbox_power_off() {
    property_tags tags;
    return set_peripheral_power_state(tags, BCMDevices::SD, false);
}

u32 SDCard::get_base_clock_freq() {
    property_tags tags;
    PropertyTagClockRate clockRate;
    clockRate.clock_id = 0x1;
    if (!tags.request_tag(0x00030002, &clockRate)) {
        return 0;
    }
    if (clockRate.clock_id != 0x1) {
        return 0;
    }
    return clockRate.rate;
}

u32 SDCard::get_clock_divider(u32 base, u32 target) {
    u32 targetted_divisor;
    if (target > base) {
        targetted_divisor = 1;
    } else {
        targetted_divisor = base / target;
        u32 mod           = base % target;
        if (mod) targetted_divisor--;
    }
    int divisor = -1;
    for (int first_bit = 31; first_bit >= 0; first_bit--) {
        u32 bit_test = (1 << first_bit);
        if (targetted_divisor & bit_test) {
            divisor = first_bit;
            targetted_divisor &= ~bit_test;
            if (targetted_divisor) {
                // The divisor is not a power-of-two, increase it
                divisor++;
            }
            break;
        }
    }
    if (divisor == -1) {
        divisor = 31;
    }
    if (divisor >= 32) {
        divisor = 31;
    }
    if (divisor != 0) {
        divisor = (1 << (divisor - 1));
    }
    if (divisor >= 0x400) {
        divisor = 0x3ff;
    }
    u32 freq_select = divisor & 0xff;
    u32 upper_bits  = (divisor >> 8) & 0x3;
    u32 ret         = (freq_select << 8) | (upper_bits << 6) | (0 << 5);
    /*
    int denominator = 1;
    if(divisor != 0)
        denominator = divisor * 2;
    int actual_clock = base_clock / denominator;
    */
    return ret;
}

bool SDCard::switch_clock_rate(u32 base, u32 target) {
    u32 divider = get_clock_divider(base, target);

    // Wait for the command inhibit (CMD and DAT) bits to clear
    while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x3) {
        bad_udelay(1000);
    }

    // Set the SD clock off
    u32 control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 &= ~(1 << 2);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bad_udelay(2000);

    // Write the new divider
    control1 &= ~0xffe0;  // Clear old setting + clock generator select
    control1 |= divider;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bad_udelay(2000);

    // Enable the SD clock
    control1 |= (1 << 2);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bad_udelay(2000);

    return true;
}

bool SDCard::reset_cmd() {
    u32 control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= SD_RESET_CMD;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    if (!timeout_wait(EMMC_BASE + EMMC_CONTROL1, SD_RESET_CMD, 0, 1000000)) {
        // TODO: Error Msg
        return false;
    }
    return true;
}

bool SDCard::reset_dat() {
    u32 control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= SD_RESET_DAT;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    if (!timeout_wait(EMMC_BASE + EMMC_CONTROL1, SD_RESET_DAT, 0, 1000000)) {
        // TODO: Error Msg
        return false;
    }
    return true;
}

void SDCard::issue_command_int(u32 cmd_reg, u32 argument, int timeout) {
    last_cmd_reg     = cmd_reg;
    last_cmd_success = 0;

    // This is as per HCSS 3.7.1.1/3.7.2.2

    // Check Command Inhibit
    while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x1) bad_udelay(1000);

    // Is the command with busy?
    if ((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) {
        // With busy

        // Is is an abort command?
        if ((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT) {
            // Not an abort command

            // Wait for the data line to be free
            while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) bad_udelay(1000);
        }
    }
    // SDMA isnt possible on Pi3
    // [Omit SDMA logic]

    // Set block size and block count
    // For now, block size = 512 bytes, block count = 1,
    //  host SDMA buffer boundary = 4 kiB
    if (blocks_to_transfer > 0xffff) {
        printf("SD: blocks_to_transfer too great (%i)\n", blocks_to_transfer);
        last_cmd_success = 0;
        return;
    }
    u32 blksizecnt = m_block_size | (blocks_to_transfer << 16);
    mmio_write(EMMC_BASE + EMMC_BLKSIZECNT, blksizecnt);

    // Set argument 1 reg
    mmio_write(EMMC_BASE + EMMC_ARG1, argument);

    // Set command reg
    mmio_write(EMMC_BASE + EMMC_CMDTM, cmd_reg);

    bad_udelay(2000);

    // Wait for command complete interrupt
    timeout_wait(EMMC_BASE + EMMC_INTERRUPT, 0x8001, 1, timeout);
    u32 irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);

    // Clear command complete status
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0001);

    // Test for errors
    if ((irpts & 0xffff0001) != 0x1) {
#ifdef EMMC_DEBUG
        printf("SD: error occured whilst waiting for command complete interrupt\n");
#endif
        last_error     = irpts & 0xffff0000;
        last_interrupt = irpts;
        return;
    }

    bad_udelay(2000);

    // Get response data
    switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
        case SD_CMD_RSPNS_TYPE_48:
        case SD_CMD_RSPNS_TYPE_48B:
            last_r0 = mmio_read(EMMC_BASE + EMMC_RESP0);
            break;

        case SD_CMD_RSPNS_TYPE_136:
            last_r0 = mmio_read(EMMC_BASE + EMMC_RESP0);
            last_r1 = mmio_read(EMMC_BASE + EMMC_RESP1);
            last_r2 = mmio_read(EMMC_BASE + EMMC_RESP2);
            last_r3 = mmio_read(EMMC_BASE + EMMC_RESP3);
            break;
    }

    // If with data, wait for the appropriate interrupt
    if ((cmd_reg & SD_CMD_ISDATA)) {
        u32 wr_irpt;
        int is_write = 0;
        if (cmd_reg & SD_CMD_DAT_DIR_CH)
            wr_irpt = (1 << 5);  // read
        else {
            is_write = 1;
            wr_irpt  = (1 << 4);  // write
        }

        int cur_block     = 0;
        auto cur_buf_addr = (u32 *)buf;
        while (cur_block < blocks_to_transfer) {
#ifdef EMMC_DEBUG
            if (blocks_to_transfer > 1)
                printf("SD: multi block transfer, awaiting block %i ready\n", cur_block);
#endif
            timeout_wait(EMMC_BASE + EMMC_INTERRUPT, (wr_irpt | 0x8000), 1, timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0000 | wr_irpt);

            if ((irpts & (0xffff0000 | wr_irpt)) != wr_irpt) {
#ifdef EMMC_DEBUG
                printf("SD: error occured whilst waiting for data ready interrupt\n");
#endif
                last_error     = irpts & 0xffff0000;
                last_interrupt = irpts;
                return;
            }

            // Transfer the block
            uSize cur_byte_no = 0;
            while (cur_byte_no < m_block_size) {
                if (is_write) {
                    u32 data = *cur_buf_addr;
                    mmio_write(EMMC_BASE + EMMC_DATA, data);
                } else {
                    u32 data      = mmio_read(EMMC_BASE + EMMC_DATA);
                    *cur_buf_addr = data;
                }
                cur_byte_no += 4;
                cur_buf_addr++;
            }

#ifdef EMMC_DEBUG
            printf("SD: block %i transfer complete\n", cur_block);
#endif

            cur_block++;
        }
    }

    // Wait for transfer complete (set if read/write transfer or with busy)
    if (((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) ||
        (cmd_reg & SD_CMD_ISDATA)) {
        // First check command inhibit (DAT) is not already 0
        if ((mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) == 0)
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
        else {
            timeout_wait(EMMC_BASE + EMMC_INTERRUPT, 0x8002, 1, timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);

            // Handle the case where both data timeout and transfer complete
            //  are set - transfer complete overrides data timeout: HCSS 2.2.17
            if (((irpts & 0xffff0002) != 0x2) && ((irpts & 0xffff0002) != 0x100002)) {
#ifdef EMMC_DEBUG
                printf("SD: error occured whilst waiting for transfer complete interrupt\n");
#endif
                last_error     = irpts & 0xffff0000;
                last_interrupt = irpts;
                return;
            }
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
        }
    }

    // Return success
    last_cmd_success = 1;
}

void SDCard::handle_card_interrupt() {
    // Handle a card interrupt

#ifdef EMMC_DEBUG
    u32 status = mmio_read(EMMC_BASE + EMMC_STATUS);

    printf("SD: card interrupt\n");
    printf("SD: controller status: %08x\n", status);
#endif

    // Get the card status
    if (card_rca) {
        issue_command_int(sd_commands[SEND_STATUS], card_rca << 16, 500000);
#ifdef EMMC_DEBUG
        if (last_cmd_success == 0) {
            printf("SD: unable to get card status\n");
        } else {
            printf("SD: card status: %08x\n", last_r0);
        }
#endif
    } else {
#ifdef EMMC_DEBUG
        printf("SD: no card currently selected\n");
#endif
    }
}

void SDCard::handle_interrupts() {
    u32 irpts      = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
    u32 reset_mask = 0;

    if (irpts & SD_COMMAND_COMPLETE) {
#ifdef EMMC_DEBUG
        printf("SD: spurious command complete interrupt\n");
#endif
        reset_mask |= SD_COMMAND_COMPLETE;
    }

    if (irpts & SD_TRANSFER_COMPLETE) {
#ifdef EMMC_DEBUG
        printf("SD: spurious transfer complete interrupt\n");
#endif
        reset_mask |= SD_TRANSFER_COMPLETE;
    }

    if (irpts & SD_BLOCK_GAP_EVENT) {
#ifdef EMMC_DEBUG
        printf("SD: spurious block gap event interrupt\n");
#endif
        reset_mask |= SD_BLOCK_GAP_EVENT;
    }

    if (irpts & SD_DMA_INTERRUPT) {
#ifdef EMMC_DEBUG
        printf("SD: spurious DMA interrupt\n");
#endif
        reset_mask |= SD_DMA_INTERRUPT;
    }

    if (irpts & SD_BUFFER_WRITE_READY) {
#ifdef EMMC_DEBUG
        printf("SD: spurious buffer write ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_WRITE_READY;
        reset_dat();
    }

    if (irpts & SD_BUFFER_READ_READY) {
#ifdef EMMC_DEBUG
        printf("SD: spurious buffer read ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_READ_READY;
        reset_dat();
    }

    if (irpts & SD_CARD_INSERTION) {
#ifdef EMMC_DEBUG
        printf("SD: card insertion detected\n");
#endif
        reset_mask |= SD_CARD_INSERTION;
    }

    if (irpts & SD_CARD_REMOVAL) {
#ifdef EMMC_DEBUG
        printf("SD: card removal detected\n");
#endif
        reset_mask |= SD_CARD_REMOVAL;
        card_removal = 1;
    }

    if (irpts & SD_CARD_INTERRUPT) {
#ifdef EMMC_DEBUG
        printf("SD: card interrupt detected\n");
#endif
        handle_card_interrupt();
        reset_mask |= SD_CARD_INTERRUPT;
    }

    if (irpts & 0x8000) {
#ifdef EMMC_DEBUG
        printf("SD: spurious error interrupt: %08x\n", irpts);
#endif
        reset_mask |= 0xffff0000;
    }

    mmio_write(EMMC_BASE + EMMC_INTERRUPT, reset_mask);
}

void SDCard::issue_command(u32 command, u32 argument, int timeout) {
    // First, handle any pending interrupts
    handle_interrupts();

    // Stop the command issue if it was the card remove interrupt that was
    //  handled
    if (card_removal) {
        last_cmd_success = 0;
        return;
    }

    // Now run the appropriate commands by calling sd_issue_command_int()
    if (command & IS_APP_CMD) {
        command &= 0xff;
#ifdef EMMC_DEBUG
        printf("SD: issuing command ACMD%i\n", command);
#endif

        if (sd_acommands[command] == SD_CMD_RESERVED(0)) {
            printf("SD: invalid command ACMD%i\n", command);
            last_cmd_success = 0;
            return;
        }
        last_cmd = APP_CMD;

        u32 rca = 0;
        if (card_rca) rca = card_rca << 16;
        issue_command_int(sd_commands[APP_CMD], rca, timeout);
        if (last_cmd_success) {
            last_cmd = command | IS_APP_CMD;
            issue_command_int(sd_acommands[command], argument, timeout);
        }
    } else {
#ifdef EMMC_DEBUG
        printf("SD: issuing command CMD%i\n", command);
#endif

        if (sd_commands[command] == SD_CMD_RESERVED(0)) {
            printf("SD: invalid command CMD%i\n", command);
            last_cmd_success = 0;
            return;
        }

        last_cmd = command;
        issue_command_int(sd_commands[command], argument, timeout);
    }

#ifdef EMMC_DEBUG
    if (last_cmd_success == 0) {
        printf("SD: error issuing command: interrupts %08x: ", last_interrupt);
        if (last_error == 0)
            printf("TIMEOUT");
        else {
            for (int i = 0; i < SD_ERR_RSVD; i++) {
                if (last_error & (1 << (i + 16))) {
                    printf(err_irpts[i]);
                    printf(" ");
                }
            }
        }
        printf("\n");
    } else
        printf("SD: command completed successfully\n");
#endif
}

int SDCard::card_init() {
    // Check the sanity of the sd_commands and sd_acommands structures
    static_assert(sizeof(sd_commands) == (64 * sizeof(u32)), "EMMC: sd_commands of incorrect size");
    static_assert(sizeof(sd_acommands) == (64 * sizeof(u32)),
                  "EMMC: sd_acommands of incorrect size");

#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
    // Power cycle the card to ensure its in its startup state
    if (!mbox_power_cycle()) {
        printf("EMMC: BCM2708 controller did not power cycle successfully\n");
    }
#ifdef EMMC_DEBUG
    else
        printf("EMMC: BCM2708 controller power-cycled\n");
#endif
#endif

    // Read the controller version
    u32 ver         = mmio_read(EMMC_BASE + EMMC_SLOTISR_VER);
    u32 vendor      = ver >> 24;
    u32 sdversion   = (ver >> 16) & 0xff;
    u32 slot_status = ver & 0xff;
    printf("EMMC: vendor %x, sdversion %x, slot_status %x\n", vendor, sdversion, slot_status);
    hci_ver = sdversion;

    if (hci_ver < 2) {
#ifdef EMMC_ALLOW_OLD_SDHCI
        printf("EMMC: WARNING: old SDHCI version detected\n");
#else
        printf("EMMC: only SDHCI versions >= 3.0 are supported\n");
        return -1;
#endif
    }

    // Reset the controller
#ifdef EMMC_DEBUG
    printf("EMMC: resetting controller\n");
#endif
    u32 control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= (1 << 24);
    // Disable clock
    control1 &= ~(1 << 2);
    control1 &= ~(1 << 0);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    timeout_wait(EMMC_BASE + EMMC_CONTROL1, (0x7 << 24), 0, 1000000);
    if ((mmio_read(EMMC_BASE + EMMC_CONTROL1) & (0x7 << 24)) != 0) {
        printf("EMMC: controller did not reset properly\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    printf("EMMC: control0: %08x, control1: %08x, control2: %08x\n",
           mmio_read(EMMC_BASE + EMMC_CONTROL0), mmio_read(EMMC_BASE + EMMC_CONTROL1),
           mmio_read(EMMC_BASE + EMMC_CONTROL2));
#endif

    // Read the capabilities registers
    capabilities_0 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_0);
    capabilities_1 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_1);
#ifdef EMMC_DEBUG
    printf("EMMC: capabilities: %08x%08x\n", capabilities_1, capabilities_0);
#endif

    // Check for a valid card
#ifdef EMMC_DEBUG
    printf("EMMC: checking for an inserted card\n");
#endif
    timeout_wait((EMMC_BASE + EMMC_STATUS), (1 << 16), 1, 500000);
    u32 status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
    if ((status_reg & (1 << 16)) == 0) {
        printf("EMMC: no card inserted\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    printf("EMMC: status: %08x\n", status_reg);
#endif

    // Clear control2
    mmio_write(EMMC_BASE + EMMC_CONTROL2, 0);

    // Get the base clock rate
    u32 new_base_clock = get_base_clock_freq();
    if (new_base_clock == 0) {
        printf("EMMC: assuming clock rate to be 100MHz\n");
        new_base_clock = 100000000;
    }

    // Set clock rate to something slow
#ifdef EMMC_DEBUG
    printf("EMMC: setting clock rate\n");
#endif
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= 1;  // enable clock

    // Set to identification frequency (400 kHz)
    u32 f_id = get_clock_divider(new_base_clock, SD_CLOCK_ID);
    if (f_id == SD_GET_CLOCK_DIVIDER_FAIL) {
        printf("EMMC: unable to get a valid clock divider for ID frequency\n");
        return -1;
    }
    control1 |= f_id;

    control1 |= (7 << 16);  // data timeout = TMCLK * 2^10
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    timeout_wait((EMMC_BASE + EMMC_CONTROL1), 0x2, 1, 0x1000000);
    if ((mmio_read(EMMC_BASE + EMMC_CONTROL1) & 0x2) == 0) {
        printf("EMMC: controller's clock did not stabilise within 1 second\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    printf("EMMC: control0: %08x, control1: %08x\n", mmio_read(EMMC_BASE + EMMC_CONTROL0),
           mmio_read(EMMC_BASE + EMMC_CONTROL1));
#endif

    // Enable the SD clock
#ifdef EMMC_DEBUG
    printf("EMMC: enabling SD clock\n");
#endif
    bad_udelay(2000);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= 4;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bad_udelay(2000);
#ifdef EMMC_DEBUG
    printf("EMMC: SD clock enabled\n");
#endif

    // Mask off sending interrupts to the ARM
    mmio_write(EMMC_BASE + EMMC_IRPT_EN, 0);
    // Reset interrupts
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);
    // Have all interrupts sent to the INTERRUPT register
    u32 irpt_mask = 0xffffffff & (~SD_CARD_INTERRUPT);
#ifdef SD_CARD_INTERRUPTS
    irpt_mask |= SD_CARD_INTERRUPT;
#endif
    mmio_write(EMMC_BASE + EMMC_IRPT_MASK, irpt_mask);

#ifdef EMMC_DEBUG
    printf("EMMC: interrupts disabled\n");
#endif
    bad_udelay(2000);

    card_supports_sdhc = 0;
    card_supports_18v  = 0;
    card_ocr           = 0;
    card_rca           = 0;
    last_interrupt     = 0;
    last_error         = 0;
    // TODO: SCR?
    failed_voltage_switch = 0;
    last_cmd_reg          = 0;
    last_cmd              = 0;
    last_cmd_success      = 0;
    last_r0               = 0;
    last_r1               = 0;
    last_r2               = 0;
    last_r3               = 0;
    this->base_clock      = new_base_clock;
    buf                   = nullptr;
    blocks_to_transfer    = 0;
    m_block_size          = 0;
    card_removal          = 0;

#ifdef EMMC_DEBUG
    printf("EMMC: device structure created\n");
#endif

    // Send CMD0 to the card (reset to idle state)
    issue_command(GO_IDLE_STATE, 0, 500000);
    if (FAIL) {
        printf("SD: no CMD0 response\n");
        return -1;
    }

    // Send CMD8 to the card
    // Voltage supplied = 0x1 = 2.7-3.6V (standard)
    // Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
#ifdef EMMC_DEBUG
    printf(
        "SD: note a timeout error on the following command (CMD8) is normal "
        "and expected if the SD card version is less than 2.0\n");
#endif
    issue_command(SEND_IF_COND, 0x1aa, 500000);
    int v2_later = 0;
    if (TIMEOUT)
        v2_later = 0;
    else if (CMD_TIMEOUT) {
        if (!reset_cmd()) return -1;
        mmio_write(EMMC_BASE + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        v2_later = 0;
    } else if (FAIL) {
        printf("SD: failure sending CMD8 (%08x)\n", last_interrupt);
        return -1;
    } else {
        if ((last_r0 & 0xfff) != 0x1aa) {
            printf("SD: unusable card\n");
#ifdef EMMC_DEBUG
            printf("SD: CMD8 response %08x\n", last_r0);
#endif
            return -1;
        } else
            v2_later = 1;
    }

    // Here we are supposed to check the response to CMD5 (HCSS 3.6)
    // It only returns if the card is a SDIO card
#ifdef EMMC_DEBUG
    printf(
        "SD: note that a timeout error on the following command (CMD5) is "
        "normal and expected if the card is not a SDIO card.\n");
#endif
    issue_command(IO_SET_OP_COND, 0, 10000);
    if (!TIMEOUT) {
        if (CMD_TIMEOUT) {
            if (!reset_cmd()) return -1;
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        } else {
            printf("SD: SDIO card detected - not currently supported\n");
#ifdef EMMC_DEBUG
            printf("SD: CMD5 returned %08x\n", last_r0);
#endif
            return -1;
        }
    }

    // Call an inquiry ACMD41 (voltage window = 0) to get the OCR
#ifdef EMMC_DEBUG
    printf("SD: sending inquiry ACMD41\n");
#endif
    issue_command(ACMD(41), 0, 500000);
    if (FAIL) {
        printf("SD: inquiry ACMD41 failed\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    printf("SD: inquiry ACMD41 returned %08x\n", last_r0);
#endif

    // Call initialization ACMD41
    int card_is_busy = 1;
    while (card_is_busy) {
        u32 v2_flags = 0;
        if (v2_later) {
            // Set SDHC support
            v2_flags |= (1 << 30);

            // Set 1.8v support
#ifdef SD_1_8V_SUPPORT
            if (!failed_voltage_switch) v2_flags |= (1 << 24);
#endif

                // Enable SDXC maximum performance
#ifdef SDXC_MAXIMUM_PERFORMANCE
            v2_flags |= (1 << 28);
#endif
        }

        issue_command(ACMD(41), 0x00ff8000 | v2_flags, 500000);
        if (FAIL) {
            printf("SD: error issuing ACMD41\n");
            return -1;
        }

        if ((last_r0 >> 31) & 0x1) {
            // Initialization is complete
            card_ocr           = (last_r0 >> 8) & 0xffff;
            card_supports_sdhc = (last_r0 >> 30) & 0x1;

#ifdef SD_1_8V_SUPPORT
            if (!failed_voltage_switch) card_supports_18v = (last_r0 >> 24) & 0x1;
#endif

            card_is_busy = 0;
        } else {
            // Card is still busy
#ifdef EMMC_DEBUG
            printf("SD: card is busy, retrying\n");
#endif
            bad_udelay(500000);
        }
    }

#ifdef EMMC_DEBUG
    printf("SD: card identified: OCR: %04x, 1.8v support: %i, SDHC support: %i\n", card_ocr,
           card_supports_18v, card_supports_sdhc);
#endif

    // At this point, we know the card is definitely an SD card, so will definitely
    //  support SDR12 mode which runs at 25 MHz
    switch_clock_rate(new_base_clock, SD_CLOCK_NORMAL);

    // A small wait before the voltage switch
    bad_udelay(5000);

    // Switch to 1.8V mode if possible
    if (card_supports_18v) {
#ifdef EMMC_DEBUG
        printf("SD: switching to 1.8V mode\n");
#endif
        // As per HCSS 3.6.1

        // Send VOLTAGE_SWITCH
        issue_command(VOLTAGE_SWITCH, 0, 500000);
        if (FAIL) {
#ifdef EMMC_DEBUG
            printf("SD: error issuing VOLTAGE_SWITCH\n");
#endif
            failed_voltage_switch = 1;
            sd_power_off();
            // TODO: Separate func call?
            return card_init();
        }

        // Disable SD clock
        control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        control1 &= ~(1 << 2);
        mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);

        // Check DAT[3:0]
        status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
        u32 dat30  = (status_reg >> 20) & 0xf;
        if (dat30 != 0) {
#ifdef EMMC_DEBUG
            printf("SD: DAT[3:0] did not settle to 0\n");
#endif
            failed_voltage_switch = 1;
            sd_power_off();
            return card_init();
        }

        // Set 1.8V signal enable to 1
        u32 control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        control0 |= (1 << 8);
        mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);

        // Wait 5 ms
        bad_udelay(5000);

        // Check the 1.8V signal enable is set
        control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        if (((control0 >> 8) & 0x1) == 0) {
#ifdef EMMC_DEBUG
            printf("SD: controller did not keep 1.8V signal enable high\n");
#endif
            failed_voltage_switch = 1;
            sd_power_off();
            return card_init();
        }

        // Re-enable the SD clock
        control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        control1 |= (1 << 2);
        mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);

        // Wait 1 ms
        bad_udelay(10000);

        // Check DAT[3:0]
        status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
        dat30      = (status_reg >> 20) & 0xf;
        if (dat30 != 0xf) {
#ifdef EMMC_DEBUG
            printf("SD: DAT[3:0] did not settle to 1111b (%01x)\n", dat30);
#endif
            failed_voltage_switch = 1;
            sd_power_off();
            return card_init();
        }

#ifdef EMMC_DEBUG
        printf("SD: voltage switch complete\n");
#endif
    }

    // Send CMD2 to get the cards CID
    issue_command(ALL_SEND_CID, 0, 500000);
    if (FAIL) {
        printf("SD: error sending ALL_SEND_CID\n");
        return -1;
    }
    u32 card_cid_0 = last_r0;
    u32 card_cid_1 = last_r1;
    u32 card_cid_2 = last_r2;
    u32 card_cid_3 = last_r3;

#ifdef EMMC_DEBUG
    printf("SD: card CID: %08x%08x%08x%08x\n", card_cid_3, card_cid_2, card_cid_1, card_cid_0);
#endif
    device_id[0] = card_cid_0;
    device_id[1] = card_cid_1;
    device_id[2] = card_cid_2;
    device_id[3] = card_cid_3;

    // Send CMD3 to enter the data state
    issue_command(SEND_RELATIVE_ADDR, 0, 500000);
    if (FAIL) {
        printf("SD: error sending SEND_RELATIVE_ADDR\n");
        return -1;
    }

    u32 cmd3_resp = last_r0;
#ifdef EMMC_DEBUG
    printf("SD: CMD3 response: %08x\n", cmd3_resp);
#endif

    card_rca        = (cmd3_resp >> 16) & 0xffff;
    u32 crc_error   = (cmd3_resp >> 15) & 0x1;
    u32 illegal_cmd = (cmd3_resp >> 14) & 0x1;
    u32 error       = (cmd3_resp >> 13) & 0x1;
    u32 status      = (cmd3_resp >> 9) & 0xf;
    u32 ready       = (cmd3_resp >> 8) & 0x1;

    if (crc_error) {
        printf("SD: CRC error\n");
        return -1;
    }

    if (illegal_cmd) {
        printf("SD: illegal command\n");
        return -1;
    }

    if (error) {
        printf("SD: generic error\n");
        return -1;
    }

    if (!ready) {
        printf("SD: not ready for data\n");
        return -1;
    }

#ifdef EMMC_DEBUG
    printf("SD: RCA: %04x\n", card_rca);
#endif

    // Now select the card (toggles it to transfer state)
    issue_command(SELECT_CARD, card_rca << 16, 500000);
    if (FAIL) {
        printf("SD: error sending CMD7\n");
        return -1;
    }

    u32 cmd7_resp = last_r0;
    status        = (cmd7_resp >> 9) & 0xf;

    if ((status != 3) && (status != 4)) {
        printf("SD: invalid status (%i)\n", status);
        return -1;
    }

    // If not an SDHC card, ensure BLOCKLEN is 512 bytes
    if (!card_supports_sdhc) {
        issue_command(SET_BLOCKLEN, 512, 500000);
        if (FAIL) {
            printf("SD: error sending SET_BLOCKLEN\n");
            return -1;
        }
    }
    m_block_size              = 512;
    u32 controller_block_size = mmio_read(EMMC_BASE + EMMC_BLKSIZECNT);
    controller_block_size &= (~0xfff);
    controller_block_size |= 0x200;
    mmio_write(EMMC_BASE + EMMC_BLKSIZECNT, controller_block_size);

    // Get the cards SCR register
    buf                = &scr.scr[0];
    m_block_size       = 8;
    blocks_to_transfer = 1;
    issue_command(SEND_SCR, 0, 500000);
    m_block_size = 512;
    if (FAIL) {
        printf("SD: error sending SEND_SCR\n");
        return -1;
    }

    // Determine card version
    // Note that the SCR is big-endian
    u32 scr0          = bek::swapBytes<u32>(scr.scr[0]);
    scr.sd_version    = SD_VER_UNKNOWN;
    u32 sd_spec       = (scr0 >> (56 - 32)) & 0xf;
    u32 sd_spec3      = (scr0 >> (47 - 32)) & 0x1;
    u32 sd_spec4      = (scr0 >> (42 - 32)) & 0x1;
    scr.sd_bus_widths = (scr0 >> (48 - 32)) & 0xf;
    if (sd_spec == 0)
        scr.sd_version = SD_VER_1;
    else if (sd_spec == 1)
        scr.sd_version = SD_VER_1_1;
    else if (sd_spec == 2) {
        if (sd_spec3 == 0)
            scr.sd_version = SD_VER_2;
        else if (sd_spec3 == 1) {
            if (sd_spec4 == 0)
                scr.sd_version = SD_VER_3;
            else if (sd_spec4 == 1)
                scr.sd_version = SD_VER_4;
        }
    }

#ifdef EMMC_DEBUG
    printf("SD: &scr: %08x\n", &scr.scr[0]);
    printf("SD: SCR[0]: %08x, SCR[1]: %08x\n", scr.scr[0], scr.scr[1]);
    printf("SD: SCR: %08x%08x\n", byte_swap32(scr.scr[0]), byte_swap32(scr.scr[1]));
    printf("SD: SCR: version %s, bus_widths %01x\n", sd_versions[scr.sd_version],
           scr.sd_bus_widths);
#endif

    if (scr.sd_bus_widths & 0x4) {
        // Set 4-bit transfer mode (ACMD6)
        // See HCSS 3.4 for the algorithm
#ifdef SD_4BIT_DATA
#ifdef EMMC_DEBUG
        printf("SD: switching to 4-bit data mode\n");
#endif

        // Disable card interrupt in host
        u32 old_irpt_mask = mmio_read(EMMC_BASE + EMMC_IRPT_MASK);
        u32 new_iprt_mask = old_irpt_mask & ~(1 << 8);
        mmio_write(EMMC_BASE + EMMC_IRPT_MASK, new_iprt_mask);

        // Send ACMD6 to change the card's bit mode
        issue_command(SET_BUS_WIDTH, 0x2, 500000);
        if (FAIL)
            printf("SD: switch to 4-bit data mode failed\n");
        else {
            // Change bit mode for Host
            u32 control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
            control0 |= 0x2;
            mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);

            // Re-enable card interrupt in host
            mmio_write(EMMC_BASE + EMMC_IRPT_MASK, old_irpt_mask);

#ifdef EMMC_DEBUG
            printf("SD: switch to 4-bit complete\n");
#endif
        }
#endif
    }

    printf("SD: found a valid version %s SD card\n", sd_versions[scr.sd_version]);
#ifdef EMMC_DEBUG
    printf("SD: setup successful (status %i)\n", status);
#endif

    // Reset interrupt register
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);

    return 0;
}

bool SDCard::mbox_power_cycle() {
    if (!mbox_power_off()) return false;

    bad_udelay(5000);

    return mbox_power_on();
}

void SDCard::sd_power_off() {
    /* Power off the SD card */
    u32 control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
    control0 &= ~(1 << 8);  // Set SD Bus Power bit off in Power Control Register
    mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);
}

int SDCard::ensure_data_mode() {
    if (card_rca == 0) {
        // Try again to initialise the card
        int ret = card_init();
        if (ret != 0) return ret;
    }

#ifdef EMMC_DEBUG
    printf("SD: ensure_data_mode() obtaining status register for card_rca %08x: ", card_rca);
#endif

    issue_command(SEND_STATUS, card_rca << 16, 500000);
    if (FAIL) {
        printf("SD: ensure_data_mode() error sending CMD13\n");
        card_rca = 0;
        return -1;
    }

    u32 status    = last_r0;
    u32 cur_state = (status >> 9) & 0xf;
#ifdef EMMC_DEBUG
    printf("status %i\n", cur_state);
#endif
    if (cur_state == 3) {
        // Currently in the stand-by state - select it
        issue_command(SELECT_CARD, card_rca << 16, 500000);
        if (FAIL) {
            printf("SD: ensure_data_mode() no response from CMD17\n");
            card_rca = 0;
            return -1;
        }
    } else if (cur_state == 5) {
        // In the data transfer state - cancel the transmission
        issue_command(STOP_TRANSMISSION, 0, 500000);
        if (FAIL) {
            printf("SD: ensure_data_mode() no response from CMD12\n");
            card_rca = 0;
            return -1;
        }

        // Reset the data circuit
        reset_dat();
    } else if (cur_state != 4) {
        // Not in the transfer state - re-initialise
        int ret = card_init();
        if (ret != 0) return ret;
    }

    // Check again that we're now in the correct mode
    if (cur_state != 4) {
#ifdef EMMC_DEBUG
        printf("SD: ensure_data_mode() rechecking status: ");
#endif
        issue_command(SEND_STATUS, card_rca << 16, 500000);
        if (FAIL) {
            printf("SD: ensure_data_mode() no response from CMD13\n");
            card_rca = 0;
            return -1;
        }
        status    = last_r0;
        cur_state = (status >> 9) & 0xf;

#ifdef EMMC_DEBUG
        printf("%i\n", cur_state);
#endif

        if (cur_state != 4) {
            printf(
                "SD: unable to initialise SD card to "
                "data mode (state %i)\n",
                cur_state);
            card_rca = 0;
            return -1;
        }
    }

    return 0;
}

int SDCard::do_data_command(bool write, uSize index, uSize number) {
    // PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
    if (!card_supports_sdhc) index *= 512;

    buf                = temp_block_buffer;
    blocks_to_transfer = number;

    // Decide on the command to use
    int command;
    if (write) {
        if (blocks_to_transfer > 1)
            command = WRITE_MULTIPLE_BLOCK;
        else
            command = WRITE_BLOCK;
    } else {
        if (blocks_to_transfer > 1)
            command = READ_MULTIPLE_BLOCK;
        else
            command = READ_SINGLE_BLOCK;
    }

    int retry_count = 0;
    int max_retries = 3;
    while (retry_count < max_retries) {
        issue_command(command, index, 5000000);

        if (SUCCESS)
            break;
        else {
            printf("SD: error sending CMD%i, ", command);
            printf("error = %08x.  ", last_error);
            retry_count++;
            if (retry_count < max_retries)
                printf("Retrying...\n");
            else
                printf("Giving up.\n");
        }
    }
    if (retry_count == max_retries) {
        card_rca = 0;
        return -1;
    }

    return 0;
}

bool SDCard::readBlock(unsigned long index, void *buffer, unsigned long offset,
                       unsigned long count) {
    // Check the status of the card
    if (ensure_data_mode() != 0) return -1;

#ifdef EMMC_DEBUG
    printf("SD: read() card ready, reading from block %u\n", index);
#endif

    if (do_data_command(false, index, 1) != 0) return false;
    memcpy(buffer, temp_block_buffer + offset, count);

#ifdef EMMC_DEBUG
    printf("SD: data read successful\n");
#endif
    return true;
}

bool SDCard::writeBlock(unsigned long index, const void *buffer, unsigned long offset,
                        unsigned long count) {
    // Check the status of the card
    if (ensure_data_mode() != 0) return -1;

#ifdef EMMC_DEBUG
    printf("SD: read() card ready, reading from block %u\n", index);
#endif
    if (offset != 0 || count != m_block_size) {
        if (do_data_command(false, index, 1) != 0) return false;
        memcpy(temp_block_buffer + offset, buffer, count);
    }
    if (do_data_command(true, index, 1) != 0) return false;

#ifdef EMMC_DEBUG
    printf("SD: data read successful\n");
#endif
    return true;
}

bool SDCard::init() { return card_init() == 0; }

bool SDCard::supports_writes() { return true; }

unsigned long SDCard::block_size() { return 512; }
