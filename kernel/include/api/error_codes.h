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

#ifndef BEKOS_ERROR_CODES_H
#define BEKOS_ERROR_CODES_H

#define __ENUMERATE_ERROR_CODES   \
    __EMIT_ERROR_CODE(ESUCCESS)   \
    __EMIT_ERROR_CODE(EADDRINUSE) \
    __EMIT_ERROR_CODE(EBADF)      \
    __EMIT_ERROR_CODE(EEXIST)     \
    __EMIT_ERROR_CODE(EFAIL)      \
    __EMIT_ERROR_CODE(EFAULT)     \
    __EMIT_ERROR_CODE(EFBIG)      \
    __EMIT_ERROR_CODE(EINVAL)     \
    __EMIT_ERROR_CODE(EIO)        \
    __EMIT_ERROR_CODE(ENODEV)     \
    __EMIT_ERROR_CODE(ENOENT)     \
    __EMIT_ERROR_CODE(ENOEXEC)    \
    __EMIT_ERROR_CODE(ENOMEM)     \
    __EMIT_ERROR_CODE(ENOTDIR)    \
    __EMIT_ERROR_CODE(ENOTSUP)    \
    __EMIT_ERROR_CODE(EOVERFLOW)  \
    __EMIT_ERROR_CODE(ESPIPE)

enum ErrorCode {
#define __EMIT_ERROR_CODE(CODE) CODE,
    __ENUMERATE_ERROR_CODES
#undef __EMIT_ERROR_CODE
};

#endif  // BEKOS_ERROR_CODES_H
