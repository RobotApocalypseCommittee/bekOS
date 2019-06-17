//
// Created by Joseph on 16/06/2019.
//
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


