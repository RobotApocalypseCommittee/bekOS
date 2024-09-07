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

#ifndef BEKOS_DEFERRED_CALLS_H
#define BEKOS_DEFERRED_CALLS_H

#include "library/function.h"
#include "library/kernel_error.h"

namespace deferred {

void initialise();
ErrorCode queue_call(bek::function<void()> callback);
void execute_queue();

}  // namespace deferred

#endif  // BEKOS_DEFERRED_CALLS_H
