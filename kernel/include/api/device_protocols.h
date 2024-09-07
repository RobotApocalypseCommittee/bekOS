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

#ifndef BEKOS_DEVICE_PROTOCOLS_H
#define BEKOS_DEVICE_PROTOCOLS_H

enum class DeviceProtocol {
    NoProtocol,
    /* --- Basic HID --- */
    Mouse = 0x100,
    Keyboard,
    /* --- Generic IO --- */
    CharStream = 0x200,
    /* --- Audiovisual --- */
    FramebufferProvider = 0x300,
};

#endif  // BEKOS_DEVICE_PROTOCOLS_H
