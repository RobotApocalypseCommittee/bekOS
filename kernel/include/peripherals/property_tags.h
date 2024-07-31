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

#ifndef BEKOS_PROPERTY_TAGS_H
#define BEKOS_PROPERTY_TAGS_H

#include "bek/types.h"
#include "mailbox.h"

struct PropertyTagHeader {
    u32 tag_id;
    u32 val_buffer_size;
    u32 code;
    // Here goes data + padding
} __attribute__((packed));

struct PropertyTagClockRate {
    PropertyTagHeader header = {0x00030002, 8, 0};
    u32 clock_id;
    // Response V
    u32 rate;
} __attribute__((packed));

struct PropertyTagGetMemory {
    static constexpr u32 GetArmTag = 0x00010005;
    static constexpr u32 GetVCTag = 0x00010006;

    explicit PropertyTagGetMemory(bool videocore): header{videocore ? GetVCTag : GetArmTag, 8, 0} {}
    PropertyTagHeader header;
    u32 base_addr{};
    u32 size{};
}  __attribute__((packed));

enum class BCMDevices : u32 {
    SD     = 0x0,
    UART0  = 0x1,
    UART1  = 0x2,
    USB    = 0x3,
    IC0    = 0x4,
    IC1    = 0x5,
    IC2    = 0x6,
    SPI    = 0x7,
    CCP2TX = 0x8,
};

struct PropertyTagPowerState {
    static constexpr u32 GetTag = 0x00020001;
    static constexpr u32 SetTag = 0x00028001;

    PropertyTagHeader header = {GetTag, 8, 0};
    BCMDevices device_id;

    // Get = response, Set = parameter + response
    u32 state;
#define POWER_STATE_ON 0x1
#define POWER_STATE_OFF 0x0
#define POWER_STATE_WAIT 0x2
#define POWER_STATE_NODEVICE 0x2
} __attribute__((packed));

struct PropertyTagSetFBSize {
    static constexpr u32 GetPhysTag = 0x00040003;
    static constexpr u32 SetPhysTag = 0x00048003;
    static constexpr u32 GetVirtTag = 0x00040004;
    static constexpr u32 SetVirtTag = 0x00048004;

    PropertyTagSetFBSize(bool set, bool virt, u32 width = 0, u32 height = 0)
        : header{(virt ? (set ? SetVirtTag : GetVirtTag) : (set ? SetPhysTag : GetPhysTag)), 8, 0},
          width(width),
          height(height){};

    PropertyTagHeader header;
    u32 width;
    u32 height;
} __attribute__((packed));

struct PropertyTagSetFBDepth {
    static constexpr u32 GetTag = 0x00040005;
    static constexpr u32 SetTag = 0x00048005;

    PropertyTagSetFBDepth(bool set, u32 depth = 0) : header{set ? SetTag : GetTag, 4, 0}, depth(depth) {};

    PropertyTagHeader header = {SetTag, 4, 0};
    u32 depth;
} __attribute__((packed));

struct PropertyTagGetFBPitch {
    static constexpr u32 GetTag = 0x00040008;

    PropertyTagHeader header = {GetTag, 4, 0};
    u32 pitch = 0;
} __attribute__((packed));

struct PropertyTagAllocateFB {
    static constexpr u32 Tag = 0x00040001;

    PropertyTagAllocateFB(u32 align): header{Tag, 8, 0}, base_address(align) {};

    PropertyTagHeader header;
    u32 base_address; // Align on input
    u32 allocation_size = 0;
} __attribute__((packed));

class property_tags {
public:
    property_tags();

    bool request_tags(void* tags, u32 tags_size);

    template <class T>
    ALWAYS_INLINE bool request_tag(u32 tag_id, T* tag) {
        tag->header.tag_id = tag_id;
        return request_tags(tag, sizeof(T));
    }

private:
    MailboxChannel m_mailbox;
};

bool set_peripheral_power_state(property_tags& tags, BCMDevices device, bool state,
                                bool wait = true);

#endif  // BEKOS_PROPERTY_TAGS_H
