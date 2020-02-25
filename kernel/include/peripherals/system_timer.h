//
// Created by Joseph on 24/02/2020.
//

#ifndef BEKOS_SYSTEM_TIMER_H
#define BEKOS_SYSTEM_TIMER_H

#include <stdint.h>
#include "interrupt_controller.h"

template <int n>
class system_timer{
public:
    system_timer();

    void set_interval(unsigned long microseconds);

    bcm_interrupt_handler* getHandler();

    bool handle_event();

    system_timer(const system_timer&) = delete;
    system_timer& operator=(const system_timer&) = delete;

private:
    unsigned long get_count();
    bool check_and_clear_interrupt();
    unsigned long interval = 0;
    static const uintptr_t count_reg;

    class int_handler: public bcm_interrupt_handler {
    public:
        explicit int_handler(system_timer<n>* mTimer);
        bool trigger() override;
    private:
        system_timer<n>* m_timer;
    };

    int_handler m_handler;
};

#endif //BEKOS_SYSTEM_TIMER_H
