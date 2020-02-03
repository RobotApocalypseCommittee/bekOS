//
// Created by Joseph on 29/01/2020.
//
#include "printf.h"

extern "C"
void show_unknown_int_complaint(unsigned long esr, unsigned long elr) {
    printf("Unknown Interrupt: ESR = %X, ELR = %X", esr, elr);
}