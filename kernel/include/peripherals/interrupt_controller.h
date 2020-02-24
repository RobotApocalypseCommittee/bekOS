//
// Created by Joseph on 24/02/2020.
//

#ifndef BEKOS_INTERRUPT_CONTROLLER_H
#define BEKOS_INTERRUPT_CONTROLLER_H



class interrupt_controller {
public:
    enum interrupt_type {
        SYSTEM_TIMER_1 = 1,
        SYSTEM_TIMER_3 = 3,
        ARM_TIMER = 64
    };
    void enable(interrupt_type t);
    void disable(interrupt_type t);
};

#endif //BEKOS_INTERRUPT_CONTROLLER_H
