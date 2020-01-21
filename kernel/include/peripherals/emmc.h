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

#ifndef BEKOS_EMMC_H
#define BEKOS_EMMC_H

#include <stdint.h>
#include <stddef.h>
#include "filesystem/hdevice.h"


struct SDSCR {
    uint32_t scr[2];
    uint32_t sd_bus_widths;
    int sd_version;
};
class SDCard: public hdevice {
public:
    bool init();
    int read(unsigned long start, void* l_buffer, unsigned long len) override;
    int write(unsigned long start, void* l_buffer, unsigned long len) override;

    unsigned int get_sector_size() override;

    bool supports_writes() override;

private:
    bool mbox_power_on();
    bool mbox_power_off();
    bool mbox_power_cycle();
    void sd_power_off();
    uint32_t get_base_clock_freq();
    uint32_t get_clock_divider(uint32_t base, uint32_t target);
    bool switch_clock_rate(uint32_t base, uint32_t target);
    bool reset_cmd();
    bool reset_dat();
    void issue_command_int(uint32_t cmd_reg, uint32_t argument, int timeout);
    void handle_card_interrupt();
    void handle_interrupts();
    void issue_command(uint32_t command, uint32_t argument, int timeout);
    int card_init();
    int ensure_data_mode();
    int do_data_command(int is_write, void* l_buffer, size_t buf_size, uint32_t block_no);





    uint32_t card_supports_sdhc;
    uint32_t card_supports_18v;
    uint32_t card_ocr;
    uint32_t card_rca;
    uint32_t last_interrupt;
    uint32_t last_error;

    struct SDSCR scr;

    int failed_voltage_switch;

    uint32_t last_cmd_reg;
    uint32_t last_cmd;
    uint32_t last_cmd_success;
    uint32_t last_r0;
    uint32_t last_r1;
    uint32_t last_r2;
    uint32_t last_r3;

    void *buf;
    int blocks_to_transfer;
    size_t block_size;
    int card_removal;
    uint32_t base_clock;

    static const uint32_t sd_commands[];
    static const uint32_t sd_acommands[];
    static const  char *err_irpts[];
    static const char* sd_versions[];

    uint32_t device_id[4];
};

#endif //BEKOS_EMMC_H
