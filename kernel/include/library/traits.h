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

}  // namespace detail

template <typename T>
using remove_ptr = detail::remove_ptr<T>::type;

template <typename T>
using remove_cv = detail::remove_cv<T>::type;

template <typename T>
using remove_reference = detail::remove_reference<T>::type;

template <typename T>
using remove_cv_reference = remove_cv<remove_reference<T>>;

template <typename T>
inline constexpr bool is_pointer = detail::is_pointer<remove_cv<T>>;

// Relocatable &c.

template <typename T>
inline constexpr bool is_trivially_copy_constructible = __is_trivially_constructible(T, const T&);

template <typename T>
inline constexpr bool is_trivially_move_constructible = __is_trivially_constructible(T, T&&);

template <typename T>
concept copy_constructible = requires(const T& t) { ::new T(t); };

template <typename T>
inline constexpr bool is_trivially_copyable = __is_trivially_copyable(T);

template <typename T>
concept trivially_copyable = is_trivially_copyable<T>;

template <typename T>
inline constexpr bool is_trivially_destructible = __has_trivial_destructor(T);

template <typename T>
concept trivially_destructible = is_trivially_destructible<T>;

template <typename From, typename To>
concept implictly_convertible_to = requires { declval<void (*)(To)>()(declval<From>()); };

// Integrals

template <typename T>
concept integral = requires(T t, T* p_t, void (*fn_t)(T)) {
    // An integral may be reinterpret_casted to itself, implicitly initialised from a literal 0, and
    // added to a pointer. Reference excluded by existence of pointer to T.
    reinterpret_cast<T>(t);
    fn_t(0);
    p_t + t;
};

}  // namespace bek

#endif  // BEKOS_TRAITS_H
