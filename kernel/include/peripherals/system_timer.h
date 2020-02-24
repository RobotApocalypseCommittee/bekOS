//
// Created by Joseph on 24/02/2020.
//

#ifndef BEKOS_SYSTEM_TIMER_H
#define BEKOS_SYSTEM_TIMER_H

#include <stdint.h>

template <int n>
class system_timer{
public:
    system_timer();

    void set_interval(unsigned long microseconds);

private:
    unsigned long get_count();
    bool check_and_clear_interrupt();
    unsigned long interval = 0;
    static const uintptr_t count_reg;
};

#endif //BEKOS_SYSTEM_TIMER_H
