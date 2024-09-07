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

#ifndef BEKOS_KB_H
#define BEKOS_KB_H
#include "bek/types.h"

namespace protocols::kb {

enum MessageKind {
    GetReport,
};

struct Report {
    u8 modifier_keys;
    u8 keys[7];
};

struct GetReportMessage {
    MessageKind kind;
    Report report;
};

}  // namespace protocols::kb

#endif  // BEKOS_KB_H
