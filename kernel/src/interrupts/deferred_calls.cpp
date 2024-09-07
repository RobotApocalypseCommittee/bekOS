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

#include "interrupts/deferred_calls.h"

using QueueMember = bek::function<void(), false, true>;

static QueueMember g_queue_member_pool[5] = {};

ErrorCode deferred::queue_call(bek::function<void()> callback) {
    for (auto& member : g_queue_member_pool) {
        if (!member) {
            member = QueueMember(bek::move(callback));
            return ESUCCESS;
        }
    }
    return ENOMEM;
}
void deferred::execute_queue() {
    for (auto& member : g_queue_member_pool) {
        if (member) {
            member();
            member = {};
        }
    }
}
void deferred::initialise() {
    for (auto& member : g_queue_member_pool) {
        member = {};
    }
}
