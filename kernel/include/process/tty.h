/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_TTY_H
#define BEKOS_TTY_H

#include "bek/format.h"
#include "bek/format_core.h"
#include "entity.h"
#include "library/intrusive_shared_ptr.h"

class ProcessDebugSerial : public EntityHandle {
public:
    Kind kind() const override { return Kind::Serial; }
    expected<uSize> write(u64 offset, TransactionalBuffer& buffer) override;
    SupportedOperations get_supported_operations() const override { return Write; }
};

#endif  // BEKOS_TTY_H
