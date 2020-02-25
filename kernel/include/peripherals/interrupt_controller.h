//
// Created by Joseph on 24/02/2020.
//

#ifndef BEKOS_INTERRUPT_CONTROLLER_H
#define BEKOS_INTERRUPT_CONTROLLER_H

class bcm_interrupt_handler {
public:
    virtual bool trigger() = 0;
};

class interrupt_controller {
public:
    enum interrupt_type {
        SYSTEM_TIMER_1 = 1,
        SYSTEM_TIMER_3 = 3,
        ARM_TIMER = 64,
        NONE = 255
    };
    void enable(interrupt_type t);
    void disable(interrupt_type t);

    void register_handler(interrupt_type interruptType, bcm_interrupt_handler* handler);

    bool handle();

private:
    bcm_interrupt_handler* handlers[96] = {};

};

#endif //BEKOS_INTERRUPT_CONTROLLER_H
