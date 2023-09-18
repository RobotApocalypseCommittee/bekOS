/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_ASSERTIONS_H
#define BEKOS_ASSERTIONS_H

namespace bek {

[[noreturn]] void panic();
[[noreturn]] void panic(const char* message, const char* file_name, unsigned int line);
[[noreturn]] void assertion_failed(const char* pExpr, const char* pFile, unsigned nLine);

}  // namespace bek

#ifdef NDEBUG

#define PANIC(msg_) bek::panic()

/// VERIFY always executes the expression, and panics if false.
#define VERIFY(expr_) ((expr_) ? ((void)0) : bek::panic())

/// Assert will not execute the expression when debug turned off.
#define ASSERT(expr_) ((void)0)

#else

#define PANIC(msg_) bek::panic(msg_, __FILE__, __LINE__)

/// VERIFY always executes the expression, and panics if false.
#define VERIFY(expr_) ((expr_) ? ((void)0) : bek::assertion_failed(#expr_, __FILE__, __LINE__))

/// Assert will not execute the expression when debug turned off.
#define ASSERT(expr_) ((expr_) ? ((void)0) : bek::assertion_failed(#expr_, __FILE__, __LINE__))

#endif

#endif  // BEKOS_ASSERTIONS_H
