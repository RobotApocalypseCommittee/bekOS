/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

.globl set_vector_table
set_vector_table:
    adr    x0, irq_vectors       // load VBAR_EL1 with virtual
    msr    vbar_el1, x0        // vector table address
    ret

.globl enable_interrupts
enable_interrupts:
    msr    daifclr, #2
    ret

.globl disable_interrupts
disable_interrupts:
    msr    daifset, #2
    ret