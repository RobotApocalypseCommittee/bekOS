/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_FUNCTION_H
#define BEKOS_FUNCTION_H

#include <library/own_ptr.h>
#include <library/utility.h>
namespace bek {

template <typename T>
class function;

/// A function object
/// Instance can contain function pointer, with optional context
/// A la callback Object
/// Optimised, hopefully, although involves branching guaranteed
template <typename Out, typename... In>
class function<Out(In...)> {
public:
    function() noexcept = default;
    function(nullptr_t) noexcept {};  // NOLINT(google-explicit-constructor)
    function(const function& other)
        : m_wrapper(other.m_wrapper ? other.m_wrapper->clone() : nullptr) {}

    function(function&& other) noexcept : m_wrapper(move(other.m_wrapper)) {}

    function& operator=(const function& other) {
        function(other).swap(*this);
        return *this;
    }

    function& operator=(function&& other) noexcept {
        m_wrapper = move(other.m_wrapper);
        return *this;
    }

    void swap(function& other) noexcept {
        using bek::swap;
        swap(m_wrapper, other.m_wrapper);
    }

    template <typename Fp>
    using EnableIfCallable = typename std::enable_if<std::is_invocable_r_v<Out, Fp, In...>>::type;

    template <typename Callable, class = EnableIfCallable<Callable>>
    function(Callable t_c) : m_wrapper(new concrete_wrapper<std::decay_t<Callable>>(move(t_c))) {}

    explicit operator bool() const noexcept { return !!m_wrapper; }

    Out operator()(In... args) {
        assert(m_wrapper);
        return m_wrapper->invoke(forward<In>(args)...);
    }

private:
    class abstract_wrapper {
    public:
        virtual own_ptr<abstract_wrapper> clone() = 0;
        virtual Out invoke(In&&...)               = 0;
        virtual ~abstract_wrapper()               = default;
    };

    template <typename Functor>
    class concrete_wrapper : public abstract_wrapper {
    private:
        Functor m_f;

    public:
        explicit concrete_wrapper(const Functor& t_f) : m_f(t_f) {}
        explicit concrete_wrapper(Functor&& t_f) : m_f(move(t_f)) {}
        own_ptr<abstract_wrapper> clone() override { return make_own<concrete_wrapper>(m_f); }

        Out invoke(In&&... in) override { return m_f(forward<In>(in)...); }
        ~concrete_wrapper() override = default;
    };
    own_ptr<abstract_wrapper> m_wrapper;
};

// Deduction Guides

namespace impl_deets {
template <typename F>
struct deduce_function_type {};

template <typename Obj, typename Out, typename... In>
struct deduce_function_type<Out (Obj::*)(In...)> {
    using type = Out(In...);
};
}  // namespace impl_deets

// Simple function pointer
template <typename Out, typename... In>
function(Out (*)(In...)) -> function<Out(In...)>;

// Functor with operator() - we dont bother with cv-qual yet
template <typename Ft, typename Fp = decltype(&Ft::operator())>
function(Ft) -> function<typename impl_deets::deduce_function_type<Fp>::type>;

template <typename Out, typename... In>
inline void swap(function<Out(In...)>& a, function<Out(In...)>& b) {
    a.swap(b);
}

}  // namespace bek
#endif  // BEKOS_FUNCTION_H
