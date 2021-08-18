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

#ifndef BEKOS_DWHOST_H
#define BEKOS_DWHOST_H

#include <library/types.h>
#include <library/own_ptr.h>
#include <peripherals/interrupt_controller.h>
#include "TransactionRequest.h"
#include "DWRootPort.h"
#include "usb/DWHCI.h"

enum class PortSpeed {
    Low,
    Full,
    High,
    Unknown
};

class DWHost {
public:
    DWHost(); 
    bool init();
    bool rescan_devices();

    bool schedule_transfer(bek::own_ptr<usb::TransferRequest> request);

    PortSpeed get_port_speed();
    bool is_device_connected();
    


private:
    bool init_core();
    bool init_host();
    bool reset();
    void enable_host_interrupts();
    void flush_tx_fifo(u32 n_fifo);
    void flush_rx_fifo();
    bool enable_root_port();

    bool handle_interrupts();

    u32 m_channel_count;

    bool m_root_port_initialised = {false};
    DWRootPort m_root_port;
};

#endif  // BEKOS_DWHOST_H
