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

#ifndef BEKOS_MEMORY_MANAGER_H
#define BEKOS_MEMORY_MANAGER_H

#include "library/types.h"
#include "page_allocator.h"

namespace mem {

/// A Region of mapped memory. This object automatically frees
class MappedRegion {};

class MemoryManager {};

}  // namespace mem

#endif  // BEKOS_MEMORY_MANAGER_H