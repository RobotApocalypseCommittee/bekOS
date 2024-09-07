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

#ifndef BEK_TIME_H
#define BEK_TIME_H

#include "types.h"

struct DecomposedTime {
    u32 year;
    u32 month;
    u32 day;
    u32 hour;
    u32 minute;
    u32 second;
    u32 nanosecond;
};

class UnixTimestamp {
public:
    explicit UnixTimestamp(u64 seconds, u32 nanoseconds = 0)
        : m_seconds(seconds), m_nanoseconds(nanoseconds) {}

    static UnixTimestamp from_decomposed(DecomposedTime time);

    DecomposedTime decompose() const;

    u64 seconds() const { return m_seconds; }

private:
    u64 m_seconds;
    u32 m_nanoseconds;
};

u16 dos_date_from(DecomposedTime date);
u16 dos_time_from(DecomposedTime time);
DecomposedTime datetime_from_dos(u16 dos_date, u16 dos_time);

#endif  // BEK_TIME_H
