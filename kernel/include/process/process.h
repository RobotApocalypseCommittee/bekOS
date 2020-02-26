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

#ifndef BEKOS_PROCESS_H
#define BEKOS_PROCESS_H

#include <hardtypes.h>
#include <stdint-gcc.h>
#include <library/vector.h>

#define KERNEL_STACK_SIZE 4096

struct SavedRegs {
    // These are the registers that are assumed not to change...
    u64 x19;
    u64 x20;
    u64 x21;
    u64 x22;
    u64 x23;
    u64 x24;
    u64 x25;
    u64 x26;
    u64 x27;
    u64 x28;
    u64 fp;
    u64 sp;
    u64 pc;
    // TODO: SIMD / Floating Point
};

enum class ProcessState {
    STOPPED,
    RUNNING,
    WAITING
};

// Data for a task which has a userspace and kernel space component
struct Process {
    // Saved from the switch call in kernelspace
    SavedRegs savedRegs;
    // Whether we are allowed run the scheduler on this task
    int preemptCounter;
    // Used for scheduling
    int processorTimeCounter;

    int processID;

    ProcessState state;
};

class ProcessManager {
public:
    typedef void (*process_fn)();

    ProcessManager();

    void fork(process_fn fn);

    void disablePreempt();
    void enablePreempt();

    bool schedule();

    void switchContext(Process* process);

    Process* getCurrentProcess();
private:
    vector<Process*> m_processes;
    Process* m_current;
};

#endif //BEKOS_PROCESS_H
