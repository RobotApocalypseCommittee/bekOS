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

#ifndef BEKOS_INTERFACE_H
#define BEKOS_INTERFACE_H

#include "Device.h"
#include "descriptors.h"
namespace usb {

/// USB Functionality - on object which handles either a whole device or an interface of said device.
class Functionality {

};

using DeviceMatchedFunctionality = Functionality* (const DeviceInfo&);
using InterfaceMatchedFunctionality = Functionality* (const InterfaceInfo&);


}

#endif  // BEKOS_INTERFACE_H
