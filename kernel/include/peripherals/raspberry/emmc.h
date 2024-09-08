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

#ifndef BEKOS_EMMC_H
#define BEKOS_EMMC_H

#include "bek/types.h"
#include "filesystem/block_device.h"

struct SDSCR {
    u32 scr[2];
    u32 sd_bus_widths;
    int sd_version;
};
class SDCard : public BlockDevice {
public:
    bool init();

    bool supports_writes() override;

    bool readBlock(unsigned long index, void *buffer, unsigned long offset, unsigned long count) override;

    bool writeBlock(unsigned long index, const void *buffer, unsigned long offset, unsigned long count) override;

    unsigned long block_size() override;

private:
    bool mbox_power_on();

    bool mbox_power_off();

    bool mbox_power_cycle();

    void sd_power_off();

    u32 get_base_clock_freq();

    u32 get_clock_divider(u32 base, u32 target);

    bool switch_clock_rate(u32 base, u32 target);

    bool reset_cmd();

    bool reset_dat();

    void issue_command_int(u32 cmd_reg, u32 argument, int timeout);

    void handle_card_interrupt();

    void handle_interrupts();

    void issue_command(u32 command, u32 argument, int timeout);

    int card_init();

    int ensure_data_mode();

    int do_data_command(bool write, uSize index, uSize number);

    u32 card_supports_sdhc;
    u32 card_supports_18v;
    u32 card_ocr;
    u32 card_rca;
    u32 last_interrupt;
    u32 last_error;

    struct SDSCR scr;

    int failed_voltage_switch;

    u32 last_cmd_reg;
    u32 last_cmd;
    u32 last_cmd_success;
    u32 last_r0;
    u32 last_r1;
    u32 last_r2;
    u32 last_r3;

    void *buf;
    int blocks_to_transfer;
    uSize m_block_size;
    int card_removal;
    u32 base_clock;

    static const u32 sd_commands[];
    static const u32 sd_acommands[];
    static const char *err_irpts[];
    static const char *sd_versions[];

    u32 device_id[4];

    u8 temp_block_buffer[512];
};

#endif //BEKOS_EMMC_H
