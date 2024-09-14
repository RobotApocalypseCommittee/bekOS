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

// __EMIT_ERROR_CODE takes (NAME, STRING DESCRIPTION)

#define __ENUMERATE_ERROR_CODES                                             \
    __EMIT_ERROR_CODE(ESUCCESS, Operation succeeded)                        \
    __EMIT_ERROR_CODE(EACCES, Permission denied)                            \
    __EMIT_ERROR_CODE(EADDRINUSE, Address already in use)                   \
    __EMIT_ERROR_CODE(EAGAIN, Resource temporarily unavailable)             \
    __EMIT_ERROR_CODE(EBADF, Bad entity handle slot)                        \
    __EMIT_ERROR_CODE(ECHILD, No child processes)                           \
    __EMIT_ERROR_CODE(EEXIST, File exists)                                  \
    __EMIT_ERROR_CODE(EFAIL, Failed for unknown reason)                     \
    __EMIT_ERROR_CODE(EFAULT, Bad address)                                  \
    __EMIT_ERROR_CODE(EFBIG, File too large)                                \
    __EMIT_ERROR_CODE(EINVAL, Invalid argument)                             \
    __EMIT_ERROR_CODE(EIO, Input / output error)                            \
    __EMIT_ERROR_CODE(ENODEV, No such device)                               \
    __EMIT_ERROR_CODE(ENOENT, No file or directory)                         \
    __EMIT_ERROR_CODE(ENOEXEC, Exec format error)                           \
    __EMIT_ERROR_CODE(ENOMEM, Not enough space)                             \
    __EMIT_ERROR_CODE(ENOTDIR, Not a directory)                             \
    __EMIT_ERROR_CODE(ENOTSUP, Operation not supported)                     \
    __EMIT_ERROR_CODE(EOVERFLOW, Value too large to be stored in data type) \
    __EMIT_ERROR_CODE(EPERM, Operation not permitted)                       \
    __EMIT_ERROR_CODE(ERANGE, Result too large)                             \
    __EMIT_ERROR_CODE(ESPIPE, Invalid seek)

enum ErrorCode {
#define __EMIT_ERROR_CODE(CODE, DESCRIPTION) CODE,
    __ENUMERATE_ERROR_CODES
#undef __EMIT_ERROR_CODE
};

#endif  // BEKOS_ERROR_CODES_H
