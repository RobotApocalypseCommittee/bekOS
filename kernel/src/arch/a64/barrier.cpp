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

#include "mm/barriers.h"

void mem::CompletionFlag::set() { asm volatile("stlrb %w0, %1" : : "r"(true), "Q"(m_b) : "memory"); }

void mem::CompletionFlag::unset() { asm volatile("stlrb %w0, %1" : : "r"(false), "Q"(m_b) : "memory"); }

bool mem::CompletionFlag::test() {
    bool x;
    asm volatile("ldarb %w0, %1" : "=r"(x) : "Q"(m_b) : "memory");
    return x;
}
