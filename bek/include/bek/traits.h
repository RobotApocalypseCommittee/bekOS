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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-builtins"
#include "types.h"
#ifndef BEKOS_TRAITS_H
#define BEKOS_TRAITS_H

namespace bek {

template <typename T>
auto declval() noexcept -> T;

template <typename T, typename U>
inline constexpr bool is_same = false;

template <typename T>
inline constexpr bool is_same<T, T> = true;

template <typename T, typename U>
concept same_as = is_same<T, U>;

// --- Functions and Callables ---

template <typename Fn, typename Out, typename... In>
inline constexpr bool is_callable_with_args = requires(Fn fn) {
    { fn(declval<In>()...) } -> same_as<Out>;
};

template <typename Fn, typename Out, typename... In>
concept callable_with_args = is_callable_with_args<Fn, Out, In...>;

template <typename>
inline constexpr bool is_function = false;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...)> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) const> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) volatile> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) const volatile> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...)&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) const&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) volatile&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) const volatile&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) &&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) const&&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) volatile&&> = true;
template <typename Out, typename... In>
inline constexpr bool is_function<Out(In...) const volatile&&> = true;

// --- Pointers and Reference Manipulation ---

namespace detail {
template <typename T>
struct remove_ptr {
    using type = T;
};

template <typename T>
struct remove_ptr<T*> {
    using type = T;
};

template <typename T>
struct remove_ptr<T* const> {
    using type = T;
};

template <typename T>
struct remove_ptr<T* volatile> {
    using type = T;
};

template <typename T>
struct remove_ptr<T* const volatile> {
    using type = T;
};

template <typename T>
struct remove_cv {
    using type = T;
};

template <typename T>
struct remove_cv<T const> {
    using type = T;
};

template <typename T>
struct remove_cv<T volatile> {
    using type = T;
};

template <typename T>
struct remove_cv<T const volatile> {
    using type = T;
};

template <typename T>
struct remove_volatile {
    using type = T;
};

template <typename T>
struct remove_volatile<T volatile> {
    using type = T;
};

template <class T>
struct remove_reference {
    typedef T type;
};
template <class T>
struct remove_reference<T&> {
    typedef T type;
};
template <class T>
struct remove_reference<T&&> {
    typedef T type;
};

template <typename T>
inline constexpr bool is_pointer = false;

template <typename T>
inline constexpr bool is_pointer<T*> = true;

template <typename T>
inline constexpr bool is_reference = false;

template <typename T>
inline constexpr bool is_reference<T&> = true;

template <typename T>
inline constexpr bool is_reference<T&&> = true;

}  // namespace detail

template <typename T>
using remove_ptr = detail::remove_ptr<T>::type;

template <typename T>
using remove_cv = detail::remove_cv<T>::type;

template <typename T>
using remove_volatile = detail::remove_volatile<T>::type;

template <typename T>
using remove_reference = detail::remove_reference<T>::type;

template <typename T>
using remove_cv_reference = remove_cv<remove_reference<T>>;

template <typename T>
inline constexpr bool is_pointer = detail::is_pointer<remove_cv<T>>;

template <typename T>
inline constexpr bool is_reference = detail::is_reference<T>;

template <typename T>
inline constexpr bool is_array = false;

template <typename T>
inline constexpr bool is_array<T[]> = true;

template <typename T, uSize N>
inline constexpr bool is_array<T[N]> = true;

template <typename T>
inline constexpr bool is_volatile = false;

template <typename T>
inline constexpr bool is_volatile<T volatile> = true;

// Relocatable &c.

#define BEK_EXPRESSION_TRAIT(NAME, EXPR)    \
    template <typename T>                   \
    inline constexpr bool is_##NAME = EXPR; \
    template <typename T>                   \
    concept NAME = is_##NAME<T>;

#define BEK_REQUIRES_TRAIT(NAME, REQUIRES) \
    template <typename T>                  \
    concept NAME = REQUIRES;               \
    template <typename T>                  \
    inline constexpr bool is_##NAME = NAME<T>;

#define BEK_ALIAS_TRAIT(NAME, OLDNAME)                 \
    template <typename T>                              \
    inline constexpr bool is_##NAME = is_##OLDNAME<T>; \
    template <typename T>                              \
    concept NAME = is_##NAME<T>;

BEK_EXPRESSION_TRAIT(trivially_copy_constructible, __is_trivially_constructible(T, const T&));
BEK_EXPRESSION_TRAIT(trivially_move_constructible, __is_trivially_constructible(T, T&&));
BEK_EXPRESSION_TRAIT(trivially_copy_assignable, __is_trivially_assignable(T, const T&));
BEK_EXPRESSION_TRAIT(trivially_move_assignable, __is_trivially_assignable(T, T&&));
BEK_EXPRESSION_TRAIT(trivially_copyable, __is_trivially_copyable(T));
BEK_EXPRESSION_TRAIT(trivially_destructible, __has_trivial_destructor(T));

BEK_REQUIRES_TRAIT(copy_constructible, requires(const T& t) { ::new T(t); });
BEK_REQUIRES_TRAIT(move_constructible, requires { ::new T(declval<T&&>()); });
BEK_REQUIRES_TRAIT(copy_assignable, requires(const T& t) { declval<T>() = t; });
BEK_REQUIRES_TRAIT(move_assignable, requires { declval<T>() = declval<T&&>(); });
BEK_REQUIRES_TRAIT(destructible, requires { declval<T>().~T(); });

// TODO: Actually?
BEK_ALIAS_TRAIT(nothrow_move_constructible, move_constructible);
BEK_ALIAS_TRAIT(nothrow_copy_constructible, copy_constructible);

template <typename T, typename... Args>
concept constructible = requires { ::new T(declval<Args>()...); };

template <typename T>
concept default_constructible = constructible<T>;

template <typename T>
inline constexpr bool is_trivial = __is_trivial(T);

template <typename From, typename To>
concept implictly_convertible_to = requires { declval<void (*)(To)>()(declval<From>()); };

template <typename B, typename D>
inline constexpr bool is_base_of = __is_base_of(B, D);

// Integrals

template <typename T>
concept integral = requires(T t, T* p_t, void (*fn_t)(T)) {
    // An integral may be reinterpret_casted to itself, implicitly initialised from a literal 0, and
    // added to a pointer. Reference excluded by existence of pointer to T.
    reinterpret_cast<T>(t);
    fn_t(0);
    p_t + t;
};

template <typename T>
using underlying_type = __underlying_type(T);

#define BEK_ENUM_BITWISE_OPS(ENUM_NAME)                                                 \
    ENUM_NAME operator|(ENUM_NAME a, ENUM_NAME b) {                                     \
        return static_cast<ENUM_NAME>(static_cast<bek::underlying_type<ENUM_NAME>>(a) | \
                                      static_cast<bek::underlying_type<ENUM_NAME>>(b)); \
    }                                                                                   \
    ENUM_NAME operator&(ENUM_NAME a, ENUM_NAME b) {                                     \
        return static_cast<ENUM_NAME>(static_cast<bek::underlying_type<ENUM_NAME>>(a) & \
                                      static_cast<bek::underlying_type<ENUM_NAME>>(b)); \
    }

#define BEK_ENUM_INLINE_BOOL(ENUM_NAME) \
    explicit operator bool() const { return static_cast<bek::underlying_type<ENUM_NAME>>(*this) }

}  // namespace bek

#endif  // BEKOS_TRAITS_H

#pragma clang diagnostic pop