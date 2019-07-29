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

#include <utils.h>
#include <peripherals/peripherals.h>
#include "peripherals/property_tags.h"

struct PropertyTagsBuffer {
    uint32_t buffer_size;
    uint32_t buffer_code;
#define BUFFER_CODE_REQUEST 0x0
#define BUFFER_CODE_RESPONSE_SUCCESS 0x80000000
#define BUFFER_CODE_RESPONSE_FAIL 0x80000001
    uint8_t Tags[0]; // Pointer cheat
};

#define TAG_CODE_REQUEST 0

property_tags::property_tags(): m_mailbox(8) {

}

bool property_tags::request_tag(uint32_t tag_id, void* tag, int tag_length) {

    // Set tag's own header bits
    auto header = reinterpret_cast<PropertyTagHeader*>(tag);
    header->tag_id = tag_id;
    header->val_buffer_size = tag_length - sizeof(PropertyTagHeader);
    header->code = TAG_CODE_REQUEST;

    // Now, put it into buffer
    auto buffer = reinterpret_cast<PropertyTagsBuffer*>(buffer_storage);
    buffer->buffer_code = BUFFER_CODE_REQUEST;
    // Buffer header, single tag, and end code 0x0
    buffer->buffer_size = (sizeof(PropertyTagsBuffer) + tag_length + sizeof(uint32_t));
    memcpy(tag, buffer->Tags, tag_length);
    // end byte
    buffer->Tags[tag_length] = 0;
    uint32_t bus_addr = (uint32_t)bus_address((uintptr_t)buffer_storage);
    m_mailbox.write(bus_addr);
    uint32_t result = m_mailbox.read();
    if (result != bus_addr) {
        // TODO: We have a problem
        return false;
    } else {
        if (buffer->buffer_code != BUFFER_CODE_RESPONSE_SUCCESS) {
            return false;
        } else {
            memcpy(buffer->Tags, tag, tag_length);
            return true;
        }
    }

}
