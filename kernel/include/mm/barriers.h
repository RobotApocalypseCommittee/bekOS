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

#ifndef BEKOS_BARRIERS_H
#define BEKOS_BARRIERS_H

namespace mem {

class CompletionFlag {
public:
    explicit CompletionFlag(bool b = false) : m_b(b) {}
    void set();
    void unset();
    bool test();

    void wait() {
        while (!test()) {
        }
    }

private:
    bool m_b;
};

}  // namespace mem

#endif  // BEKOS_BARRIERS_H
