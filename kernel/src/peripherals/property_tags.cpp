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

#include "peripherals/property_tags.h"
#include "library/utility.h"
#include <utils.h>
#include <peripherals/peripherals.h>

struct PropertyTagsBuffer {
    u32 buffer_size;
    u32 buffer_code;
#define BUFFER_CODE_REQUEST 0x0
#define BUFFER_CODE_RESPONSE_SUCCESS 0x80000000
#define BUFFER_CODE_RESPONSE_FAIL 0x80000001
    u8 Tags[0]; // Pointer cheat
};

#define TAG_CODE_REQUEST 0

property_tags::property_tags(): m_mailbox(8) {

}
bool property_tags::request_tags(void* tags, u32 tags_size) {
    // Header + tags + end
    u32 buffer_size = 8 + tags_size + 4;
    // Pad - must be 16 byte aligned
    buffer_size += (buffer_size % 16) ? 16 - (buffer_size % 16) : 0;

    // Hope kmalloc aligns 16
    PropertyTagsBuffer* buffer = reinterpret_cast<PropertyTagsBuffer*>(kmalloc(buffer_size));
    buffer->buffer_code = BUFFER_CODE_REQUEST;
    buffer->buffer_size = buffer_size;
    memcpy(&buffer->Tags[0], tags, tags_size);
    buffer->Tags[tags_size] = 0;
    // Ready to send
    u32 bus_addr = (u32)bus_address((uPtr)buffer);
    write_barrier();
    m_mailbox.write(bus_addr);
    u32 result = m_mailbox.read();
    read_barrier();
    assert(result == bus_addr);
    assert(buffer->buffer_code == BUFFER_CODE_RESPONSE_SUCCESS);
    memcpy(tags, buffer->Tags, tags_size);
    return true;
}
bool set_peripheral_power_state(property_tags& tags, BCMDevices device, bool state, bool wait) {
    PropertyTagPowerState tag = {.device_id = device,
                                 .state = static_cast<u32>(state) | static_cast<u32>(wait) << 1};
    if (!tags.request_tag(PropertyTagPowerState::SetTag, &tag)) {
        return false;
    }
    if (tag.device_id != device || (tag.state & 0b10) || (tag.state & 0b1) != state) {
        return false;
    }
    return true;
}
