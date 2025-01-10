// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BEKOS_KERNEL_ERROR_H
#define BEKOS_KERNEL_ERROR_H

#include "api/error_codes.h"
#include "bek/expected.h"
#include "bek/format_core.h"

template <typename T, typename E = ErrorCode>
using expected = bek::expected<T, E>;

void bek_basic_format(bek::OutputStream& out, const ErrorCode& code);

#define EXPECT_SUCCESS_MESSAGE(EXPRESSION, FAILURE_MSG)      \
    if (ErrorCode _temp = (EXPRESSION); _temp != ESUCCESS) { \
        DBG::dbgln(FAILURE_MSG ": {}"_sv, _temp.error());    \
        return _temp;                                        \
    }

#define EXPECTED_TRY_MESSAGE(EXPRESSION, FAILURE_MSG)         \
    ({                                                        \
        auto&& _temp = (EXPRESSION);                          \
        if (_temp.has_error()) {                              \
            DBG::dbgln(FAILURE_MSG ": {}"_sv, _temp.error()); \
            return _temp.error();                             \
        }                                                     \
        _temp.release_value();                                \
    })

#endif  // BEKOS_KERNEL_ERROR_H
