//
// Created by Joseph on 24/10/2020.
//
#include "usb/DWRootPort.h"
#include "usb/DWHost.h"
#include "usb/interface.h"


bool DWRootPort::init() {
    auto port_speed = m_host->get_port_speed();
    if (port_speed == PortSpeed::Unknown) {
        // TODO: Error
        return false;
    }
    
}
bool DWRootPort::rescan_devices() { return false; }
