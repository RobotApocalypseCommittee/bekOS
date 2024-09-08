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
#include "peripherals/mailbox.h"

#include "peripherals/arm/gentimer.h"
#include "peripherals/peripherals.h"
// FIXME
#define MAILBOX_BASE (0xBBBBBBBB + 0x0000B880)
#define MAILBOX0_READ (0x0)
#define MAILBOX0_STATUS (0x18)
#define MAILBOX1_WRITE (0x20)
#define MAILBOX1_STATUS (0x38)

#define MAILBOX_EMPTY 0x40000000
#define MAILBOX_FULL 0x80000000
MailboxChannel::MailboxChannel(const u32 &channel) : mmio_register_device(mapped_address(MAILBOX_BASE)), channel(channel) {}

u32 MailboxChannel::read() {
    u32 data;
    do {
        while (read_register(MAILBOX0_STATUS) & MAILBOX_EMPTY) {

        }
        data = read_register(MAILBOX0_READ);
    } while ((data & 0x0F) != channel);

    return data & ~0x0F;
}

void MailboxChannel::write(u32 data) {
    while (read_register(MAILBOX1_STATUS) & MAILBOX_FULL) {

    }

    write_register(MAILBOX1_WRITE, data | channel);
}

void MailboxChannel::flush() {
    while (!(read_register(MAILBOX0_STATUS) & MAILBOX_EMPTY)) {
        read_register(MAILBOX0_READ);
        // TODO: Replace
        timing::spindelay_us(20000);
    }

}


