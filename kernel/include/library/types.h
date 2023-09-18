/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_TYPES_H
#define BEKOS_TYPES_H

using u8  = __UINT8_TYPE__;
using u16 = __UINT16_TYPE__;
using u32 = __UINT32_TYPE__;
using u64 = __UINT64_TYPE__;
using i8  = __INT8_TYPE__;
using i16 = __INT16_TYPE__;
using i32 = __INT32_TYPE__;
using i64 = __INT64_TYPE__;

using char16 = char16_t;

using uSize = __SIZE_TYPE__;

using uPtr = __UINTPTR_TYPE__;
using iPtr = __INTPTR_TYPE__;
using ptrDiff = iPtr;

#define ALWAYS_INLINE [[gnu::always_inline]] inline

typedef decltype(nullptr) nullptr_t;

inline constexpr uSize KiB = 1024;
inline constexpr uSize MiB = 1024 * KiB;

#endif  // BEKOS_TYPES_H
