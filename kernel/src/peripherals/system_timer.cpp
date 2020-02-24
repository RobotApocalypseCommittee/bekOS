//
// Created by Joseph on 24/02/2020.
//
#include "peripherals/system_timer.h"
#include <peripherals/peripherals.h>

#define TIMER_BASE (PERIPHERAL_BASE + 0x3000)
#define TIMER_CS (TIMER_BASE)
#define TIMER_CLO (TIMER_BASE + 0x4)
#define TIMER_CHI (TIMER_BASE + 0x8)
#define TIMER_C1 (TIMER_BASE + 0x10)
#define TIMER_C3 (TIMER_BASE + 0x18)

#define TIMER_FREQUENCY 1000000

template<int n>
system_timer<n>::system_timer() {
    check_and_clear_interrupt();
}

template<int n>
void system_timer<n>::set_interval(unsigned long microseconds) {
    interval = microseconds * TIMER_FREQUENCY / 1000000;
    unsigned long cnt = get_count();
    mmio_write(count_reg, static_cast<unsigned int>(cnt) + interval);
}

template<int n>
unsigned long system_timer<n>::get_count() {
    unsigned int chi = mmio_read(TIMER_CHI);
    unsigned int clo = mmio_read(TIMER_CLO);

    while (mmio_read(TIMER_CHI) != chi) {
        chi = mmio_read(TIMER_CHI);
        clo = mmio_read(TIMER_CHI);
    }
    return ((unsigned long) chi << 32u) | clo;
}

template<int n>
bool system_timer<n>::check_and_clear_interrupt() {
    unsigned int ans = mmio_read(TIMER_CS);
    if ((ans & (1u << n)) != 0) {
        mmio_write(TIMER_CS, 1u << n);
        return true;
    }
    return false;
}

template<>
const uintptr_t system_timer<1>::count_reg = TIMER_C1;

template<>
const uintptr_t system_timer<3>::count_reg = TIMER_C3;

template class system_timer<1>;
template class system_timer<3>;

