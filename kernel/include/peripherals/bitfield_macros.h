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

#ifndef BEKOS_BITFIELD_MACROS_H
#define BEKOS_BITFIELD_MACROS_H

#define BEK_BIT(POS) (1ul << POS)

#define BEK_SET_BIT(VALUE, POS) (VALUE | (1U << POS))
#define BEK_CLEAR_BIT(VALUE, POS) (VALUE & (~(1U << POS)))
#define BEK_BIT_MASK(BITS) ((1u << BITS) - 1)

#define BEK_BIT_PRESERVE_ALL 0xFFFFFFFF
#define BEK_BIT_PRESERVE_NONE 0

#define BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS) \
    bool get_##NAME() const { return (REGISTER() >> POS) & 1; }

#define BEK_BIT_ACCESSOR_RW1S(NAME, REGISTER, POS) \
    BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS)       \
    void set_##NAME() { REGISTER(BEK_SET_BIT(REGISTER##_preserved(), POS)); }

#define BEK_BIT_ACCESSOR_RW1C(NAME, REGISTER, POS) \
    BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS)       \
    void clear_##NAME() { REGISTER(BEK_SET_BIT(REGISTER##_preserved(), POS)); }

#define BEK_BIT_ACCESSOR_RW(NAME, REGISTER, POS)                              \
    BEK_BIT_ACCESSOR_RO(NAME, REGISTER, POS)                                  \
    void set_##NAME() { REGISTER(BEK_SET_BIT(REGISTER##_preserved(), POS)); } \
    void clear_##NAME() { REGISTER(BEK_CLEAR_BIT(REGISTER##_preserved(), POS)); }

#define BEK_REGISTER_RO(NAME, TYPE, OFFSET) \
    TYPE NAME() const { return base.read<TYPE>(OFFSET); }

#define BEK_REGISTER_P(NAME, TYPE, OFFSET, PRESERVE_MASK) \
    BEK_REGISTER_RO(NAME, TYPE, OFFSET)                   \
    void NAME(TYPE v) { base.write<TYPE>(OFFSET, v); }    \
    TYPE NAME##_preserved() const { return base.read<TYPE>(OFFSET) & PRESERVE_MASK; }

#define BEK_REGISTER(NAME, TYPE, OFFSET) BEK_REGISTER_P(NAME, TYPE, OFFSET, BEK_BIT_PRESERVE_ALL)

#define BEK_BIT_FIELD_RO(NAME, REGISTER, POS, BITS, TYPE) \
    [[nodiscard]] TYPE get_##NAME() const { return (REGISTER() >> POS) & BEK_BIT_MASK(BITS); }

#define BEK_BIT_FIELD_RW(NAME, REGISTER, POS, BITS, TYPE)                  \
    BEK_BIT_FIELD_RO(NAME, REGISTER, POS, BITS, TYPE)                      \
    void set_##NAME(TYPE v) {                                              \
        REGISTER((REGISTER##_preserved() & ~(BEK_BIT_MASK(BITS) << POS)) | \
                 (static_cast<u32>(v) << POS));                            \
    }

#endif  // BEKOS_BITFIELD_MACROS_H
