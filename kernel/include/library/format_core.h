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

//
// Created by Joe Bell on 22/08/2023.
//

#ifndef BEKOS_FORMAT_CORE_H
#define BEKOS_FORMAT_CORE_H

#include "library/string.h"
#include "library/types.h"

namespace bek {

struct OutputStream {
    virtual void write(bek::str_view str) = 0;
    virtual void write(char c)            = 0;
    virtual void reserve(uSize n)         = 0;
};

}  // namespace bek

#endif  // BEKOS_FORMAT_CORE_H
