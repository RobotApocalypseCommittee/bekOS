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

#ifndef BEKOS_PROPERTY_TAGS_H
#define BEKOS_PROPERTY_TAGS_H


#include <stdint.h>
#include "mailbox.h"

struct PropertyTagHeader {
    uint32_t tag_id;
    uint32_t val_buffer_size;
    uint32_t code;
    // Here goes data + padding
} __attribute__ ((packed));


struct PropertyTagClockRate {
    PropertyTagHeader header;
    uint32_t clock_id;
    // Response V
    uint32_t rate;
} __attribute__ ((packed));

struct PropertyTagPowerState {
    PropertyTagHeader header;
    uint32_t device_id;
    // Get = response, Set = parameter + response
    uint32_t state;
#define POWER_STATE_ON 0x1
#define POWER_STATE_OFF 0x0
#define POWER_STATE_WAIT 0x2
#define POWER_STATE_NODEVICE 0x2
} __attribute__ ((packed));

class property_tags {
public:
    property_tags();

    bool request_tag(uint32_t tag_id, void* tag, int tag_length);


private:
    // TODO: THis is static, and too short for some properties - work something out
    alignas(16) uint32_t buffer_storage[256];
    MailboxChannel m_mailbox;

};


#endif //BEKOS_PROPERTY_TAGS_H
