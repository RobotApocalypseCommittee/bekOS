/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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
#include <peripherals/gentimer.h>
#include "peripherals/mailbox.h"
#include "peripherals/peripherals.h"

#define MAILBOX_BASE (PERIPHERAL_BASE + 0x0000B880)
#define MAILBOX0_READ (MAILBOX_BASE)
#define MAILBOX0_STATUS (MAILBOX_BASE + 0x18)
#define MAILBOX1_WRITE (MAILBOX_BASE+0x20)
#define MAILBOX1_STATUS (MAILBOX_BASE + 0x38)

#define MAILBOX_EMPTY 0x40000000
#define MAILBOX_FULL 0x80000000
MailboxChannel::MailboxChannel(const uint32_t &channel) : channel(channel) {}

uint32_t MailboxChannel::read() {
    uint32_t data;
    do {
        while (mmio_read(MAILBOX0_STATUS) & MAILBOX_EMPTY) {

        }
        data = mmio_read(MAILBOX0_READ);
    } while ((data & 0x0F) != channel);

    return data & ~0x0F;
}

void MailboxChannel::write(uint32_t data) {
    while (mmio_read(MAILBOX1_STATUS) & MAILBOX_FULL) {

    }

    mmio_write(MAILBOX1_WRITE, data | channel);
}

void MailboxChannel::flush() {
    while (!(mmio_read(MAILBOX0_STATUS) & MAILBOX_EMPTY)) {
        mmio_read(MAILBOX0_READ);
        // TODO: Replace
        bad_udelay(20000);
    }

}


