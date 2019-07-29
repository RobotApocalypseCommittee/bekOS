/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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

#include "peripherals/gentimer.h"

unsigned long getClockFrequency() {
    unsigned long nCNTFRQ;
    asm volatile ("mrs %0, CNTFRQ_EL0" : "=r" (nCNTFRQ));
    return nCNTFRQ;
}

unsigned long getClockTicks() {
    unsigned long nCNTPCT;
    asm volatile ("mrs %0, CNTPCT_EL0" : "=r" (nCNTPCT));
    return nCNTPCT;
}

void bad_udelay(unsigned int microseconds) {
    unsigned long clockticks = microseconds * getClockFrequency();
    clockticks /= 1000000;
    clockticks += getClockTicks();
    while (getClockTicks() < clockticks) {
        // Do Nothing
    }
}


