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

#ifndef BEKOS_MAILBOX_H
#define BEKOS_MAILBOX_H

#include "bek/types.h"
#include "peripherals.h"

class MailboxChannel: mmio_register_device {
public:
    explicit MailboxChannel(const u32 &channel);
    u32 read();
    void write(u32 data);
    void flush();

private:
    u32 channel;
};

#endif //BEKOS_MAILBOX_H
