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

#include "bek/time.h"

#include "bek/assertions.h"

constexpr u64 MONTH_DAYS[]    = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
constexpr u64 SECONDS_IN_YEAR = 31556952;
constexpr u64 SECONDS_IN_DAY  = 60 * 60 * 24;

constexpr bool is_leap_year(u64 year) {
    return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

UnixTimestamp UnixTimestamp::from_decomposed(DecomposedTime time) {
    if (time.year < 1970) {
        time.year  = 1970;
        time.month = 1;
        time.day   = 1;
    }

    u64 days = 365 * (time.year - 1970);
    // Leap years
    days += (time.year - 1) / 4 - ((1970 - 1) / 4);
    days -= (time.year - 1) / 100 - ((1970 - 1) / 100);
    days += (time.year - 1) / 400 - ((1970 - 1) / 400);

    if (time.month > 12 || time.month < 1) {
        time.month = 1;
    }
    days += MONTH_DAYS[time.month - 1];
    if (time.month >= 3 && time.year % 4 == 0 && (time.year % 100 != 0 || time.year % 400 == 0)) {
        days += 1;
    }
    if (time.day < 1 || time.day > 31) {
        time.day = 1;
    }
    days += time.day - 1;
    return UnixTimestamp{days * SECONDS_IN_DAY + time.hour * 3600 + time.minute * 60 + time.second,
                         time.nanosecond};
}
DecomposedTime UnixTimestamp::decompose() const {
    u64 years           = m_seconds / SECONDS_IN_YEAR;
    u32 leftover        = (u32)(m_seconds % SECONDS_IN_YEAR);
    u32 leftover_days   = leftover / SECONDS_IN_DAY;
    u32 leftover_in_day = leftover % SECONDS_IN_DAY;
    auto year           = (u32)years + 1970;
    bool leap_year      = is_leap_year(year);
    u32 month           = 1;
    for (; month <= 12; month++) {
        auto min_days = MONTH_DAYS[month];
        if (leap_year && month >= 3) {
            min_days + 1;
        }
        if (leftover_days < min_days) {
            break;
        }
    }
    u32 days    = leftover_days - MONTH_DAYS[month - 1];
    u32 hours   = leftover_in_day / 3600;
    u32 minutes = (leftover_in_day % 3600) / 60;
    u32 seconds = leftover_in_day % 60;
    return {year, month, days, hours, minutes, seconds, m_nanoseconds};
}

union DOSPackedTime {
    u16 value;
    struct {
        u16 second : 5;
        u16 minute : 6;
        u16 hour : 5;
    };
};
static_assert(sizeof(DOSPackedTime) == 2);

union DOSPackedDate {
    u16 value;
    struct {
        u16 day : 5;
        u16 month : 4;
        u16 year : 7;
    };
};
static_assert(sizeof(DOSPackedDate) == 2);

u16 dos_date_from(DecomposedTime date) {
    DOSPackedDate result;
    result.year  = date.year - 1980;
    result.month = date.month;
    result.day   = date.day;
    return result.value;
}
u16 dos_time_from(DecomposedTime time) {
    DOSPackedTime result;
    result.hour   = time.hour;
    result.minute = time.minute;
    result.second = time.second / 2;
    return result.value;
}
DecomposedTime datetime_from_dos(u16 dos_date, u16 dos_time) {
    DOSPackedTime time = {.value = dos_time};
    DOSPackedDate date = {.value = dos_date};
    return {static_cast<u32>(date.year + 1980),
            static_cast<u32>(dos_date ? date.month : 1),
            static_cast<u32>(dos_date ? date.day : 1),
            time.hour,
            time.minute,
            static_cast<u32>(time.second * 2),
            0};
}
