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

.globl memzero
memzero:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt memzero
    ret

.globl set_usertable
set_usertable:
    msr	ttbr0_el1, x0
    tlbi vmalle1is // clear caches
    DSB ISH // wait for data to be done
    isb // wait for instruction to be done
    ret

.globl blink
blink:
	mov x0, #0x3f200000
	movk    x0, 0xffff, lsl 48
	mov x1, #0x1
	mov x2, #0x7
	lsl x1, x1, 27
	lsl x2, x2, 27
	ldr w3, [x0, #8]     // Read function mode
	bic x3, x3, x2
	orr x3, x3, x1
	str w3, [x0, #8]    // Write function mode back
	mov x1, #0x1
	lsl x1, x1, #29
flash:
	str w1, [x0, #40]	// Hit the clear register
	mov x2, #0x800000
1:
	sub x2, x2, #1
	cmp x2, #0
	bne 1b
	str w1, [x0, #28]	// Hit the set register
	mov x2, #0x800000
2:
	sub x2, x2, #1
	cmp x2, #0
	bne 2b
	b flash

.globl nootdata
nootdata:
.quad 0xffff00003f200000