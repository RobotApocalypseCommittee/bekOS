/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2025 Bekos Contributors
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

#ifndef BEKOS_API_MOUSE_H
#define BEKOS_API_MOUSE_H

#include "bek/types.h"

namespace protocols::mouse {

enum MessageKind {
    GetReport,
};

struct Report {
    enum : u8 {
        Button1 = 1,
        Button2 = 2,
        Button3 = 4,
        Button4 = 8,
        Button5 = 16,
    } buttons;
    i32 delta_x;
    i32 delta_y;
    u8 sequence_number;
};

struct GetReportMessage {
    MessageKind kind;
    Report report;
};

}  // namespace protocols::mouse

#endif  // BEKOS_API_MOUSE_H
