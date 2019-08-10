

#ifndef BEKOS_EMMC_H
#define BEKOS_EMMC_H

#include <stdint.h>
#include <stddef.h>
#include "types.h"


struct SDSCR {
    uint32_t scr[2];
    uint32_t sd_bus_widths;
    int sd_version;
};
class SDCard {
public:
    SDCard();
    ~SDCard();
    bool init();
    int read(unsigned long start, void* buf, unsigned int len);
    int write(unsigned long start, const void* buf, unsigned int len);
private:
    bool mbox_power_on();
    bool mbox_power_off();
    bool mbox_power_cycle();
    void sd_power_off();
    int do_data_command();
    u32 get_base_clock_freq();
    u32 get_clock_divider(u32 base, u32 target);
    bool switch_clock_rate(u32 base, u32 target);
    bool reset_cmd();
    bool reset_dat();
    void issue_command_int(uint32_t cmd_reg, uint32_t argument, int timeout);
    void handle_card_interrupt();
    void handle_interrupts();
    void issue_command(uint32_t command, uint32_t argument, int timeout);
    int card_init();
    int ensure_data_mode();
    int do_data_command(int is_write, uint8_t* buf, size_t buf_size, uint32_t block_no);





    uint32_t card_supports_sdhc;
    uint32_t card_supports_18v;
    uint32_t card_ocr;
    uint32_t card_rca;
    uint32_t last_interrupt;
    uint32_t last_error;

    struct SDSCR *scr;

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






};

#endif //BEKOS_EMMC_H
