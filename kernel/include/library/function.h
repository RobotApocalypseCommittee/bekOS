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

#ifndef BEKOS_FUNCTION_H
#define BEKOS_FUNCTION_H

#include "bek/own_ptr.h"
#include "bek/traits.h"
#include "bek/utility.h"

namespace bek {

namespace detail {

// Using standard T&& for ints forces passing by ref and moving (no-op) rather than pass by value.
// Could use more convoluted logic to allow small trivially copyable structs also.
template <typename T>
using fast_forward_t = bek::conditional<bek::integral<T>, T, T&&>;

namespace function {

static constexpr uSize STORAGE_SIZE  = 16;
static constexpr uSize STORAGE_ALIGN = 8;

using Storage = aligned_storage<STORAGE_SIZE, STORAGE_ALIGN>;

template <typename Functor>
static constexpr bool uses_trivial_small_storage = bek::is_trivially_copyable<Functor> && Storage::can_store<Functor>;

struct abstract_wrapper {
    virtual abstract_wrapper* clone(Storage& storage)   = 0;
    virtual abstract_wrapper* move_to(Storage& storage) = 0;
    virtual ~abstract_wrapper()                         = default;
};

template <typename Functor>
struct concrete_wrapper final : abstract_wrapper {
    static_assert(!uses_trivial_small_storage<Functor>);

    Functor m_functor;
    concrete_wrapper(Functor&& functor) : m_functor(bek::forward<Functor&&>(functor)) {}
    concrete_wrapper(const Functor& functor)
        requires bek::copy_constructible<Functor>
        : m_functor(functor) {}
    concrete_wrapper(const concrete_wrapper&)            = delete;
    concrete_wrapper(concrete_wrapper&&)                 = delete;
    concrete_wrapper& operator=(const concrete_wrapper&) = delete;
    concrete_wrapper& operator=(concrete_wrapper&&)      = delete;

    abstract_wrapper* clone(Storage& storage) override {
        if constexpr (bek::copy_constructible<Functor>) {
            if constexpr (Storage::can_store<concrete_wrapper>) {
                return new (storage.data) concrete_wrapper(m_functor);
            } else {
                return new concrete_wrapper(m_functor);
            }
        } else {
            bek::panic();
        }
    }

    abstract_wrapper* move_to(Storage& storage) override {
        if constexpr (Storage::can_store<concrete_wrapper>) {
            return new (storage.data) concrete_wrapper(bek::move(m_functor));
        } else {
            // Never called for external storage - just twiddle ptrs.
            return nullptr;
        }
    }
    ~concrete_wrapper() override = default;
};

}  // namespace function
}  // namespace detail

template <typename T, bool Copyable = false, bool Nullable = false>
class function;

namespace detail {
template <typename T>
inline constexpr bool is_bek_function = false;

template <typename T, bool Copyable, bool Nullable>
inline constexpr bool is_bek_function<bek::function<T, Copyable, Nullable>> = true;

template <typename T>
concept bek_function = is_bek_function<T>;

}  // namespace detail

/// A function object
/// Instance can contain function pointer, with optional context
/// A la callback Object
/// Optimised, hopefully, although involves branching guaranteed
template <typename Out, typename... In, bool Copyable, bool Nullable>
class function<Out(In...), Copyable, Nullable> {
    using InvokerPtr = Out (*)(void*, detail::fast_forward_t<In>...);
    static_assert(sizeof(InvokerPtr) == sizeof(void*));
    static_assert(alignof(InvokerPtr) == alignof(void*));
    using Storage = detail::function::Storage;

    template <typename T, bool C, bool N>
    friend class function;

public:
    using Signature = Out(In...);
    function()
        requires(Nullable)
    = default;

    explicit operator bool() const { return m_invoker != &empty_function; }

    /// Construct a function with a given Functor, which must be callable with the appropriate
    /// arguments, and copy-constructible.
    template <bek::callable_with_args<Out, In...> Functor>
        requires(!detail::is_bek_function<remove_cv_reference<Functor>> &&
                 (copy_constructible<remove_cv_reference<Functor>> || !Copyable))
    explicit(!detail::function::uses_trivial_small_storage<Functor>)
        function(Functor&& functor) {  // NOLINT(*-forwarding-reference-overload, *-explicit-constructor)
        // We're forwarding these references.
        if constexpr (detail::function::uses_trivial_small_storage<Functor>) {
            new (m_storage.ptr()) Functor(static_cast<Functor&&>(functor));
            m_wrapper = nullptr;

            m_invoker = [](void* p_fn, detail::fast_forward_t<In>... in) {
                auto* fn = reinterpret_cast<Functor*>(p_fn);
                return (*fn)(forward<In>(in)...);
            };
        } else {
            // We use a wrapper.
            using wrapper_t = detail::function::concrete_wrapper<Functor>;

            if constexpr (Storage::can_store<wrapper_t>) {
                m_wrapper = new (m_storage.ptr()) wrapper_t(static_cast<Functor&&>(functor));
            } else {
                m_wrapper = new wrapper_t(static_cast<Functor&&>(functor));
            }

            m_invoker = [](void* p_fn, detail::fast_forward_t<In>... in) {
                auto& fn = reinterpret_cast<wrapper_t*>(p_fn)->m_functor;
                return fn(forward<In>(in)...);
            };
        }
    }

    template <bool OtherCopyable, bool OtherNullable>
        requires((Nullable && !Copyable && (!OtherNullable || OtherCopyable)) ||
                 (!OtherNullable && OtherCopyable && (!Copyable || Nullable)))
    explicit function(bek::function<Signature, OtherCopyable, OtherNullable>&& other) {
        do_swap(*this, other);
    }

    function(const function& other)
        requires Copyable
        : m_invoker{other.m_invoker}, m_wrapper{nullptr} {
        if (other.m_wrapper) {
            m_wrapper = other.m_wrapper->clone(m_storage);
        } else {
            m_storage = other.m_storage;
        }
    }

    function(function&& other) noexcept : m_invoker{other.m_invoker} {
        if (other.m_wrapper) {
            if (other.is_inline_wrapped()) {
                m_wrapper = other.m_wrapper->move_to(m_storage);
            } else {
                m_wrapper       = other.m_wrapper;
                other.m_wrapper = nullptr;
            }
        } else {
            // Move is bitwise copy.
            m_wrapper = nullptr;
            m_storage = other.m_storage;
        }
    }

    function& operator=(function other) {
        do_swap(*this, other);
        return *this;
    }

    friend void swap(function& a, function& b) { do_swap(a, b); }


    Out operator()(In... args) {
        return m_invoker((m_wrapper) ? m_wrapper : m_storage.ptr(), bek::forward<In>(args)...);
    }

    ~function() {
        if (is_inline_wrapped()) {
            m_wrapper->~abstract_wrapper();
        } else {
            // If null-ptr, does nothing.
            delete m_wrapper;
        }
    }

private:
    template <bool OtherCopyable, bool OtherNullable>
    static void do_swap(function& a, bek::function<Signature, OtherCopyable, OtherNullable>& b) noexcept {
        using bek::swap;
        swap(a.m_storage, b.m_storage);
        swap(a.m_invoker, b.m_invoker);
        // Can't just swap m_wrapper.
        auto b_wrapper = b.m_wrapper;
        if (a.m_wrapper == a.m_storage.ptr()) {
            b.m_wrapper = static_cast<detail::function::abstract_wrapper*>(b.m_storage.ptr());
        } else {
            b.m_wrapper = a.m_wrapper;
        }
        if (b_wrapper == b.m_storage.ptr()) {
            a.m_wrapper = static_cast<detail::function::abstract_wrapper*>(a.m_storage.ptr());
        } else {
            a.m_wrapper = b_wrapper;
        }
    }

    [[nodiscard]] constexpr inline bool is_inline_wrapped() const { return m_storage.ptr() == m_wrapper; }

    static Out empty_function(void*, detail::fast_forward_t<In>...) { PANIC("Called empty bek::function"); }

    // Members
    Storage m_storage;

    /// Type-erasing function to invoke stored functor.
    InvokerPtr m_invoker{&empty_function};

    /// Points either to m_storage or some external place, or is nullptr (fn ptr) or is 1 (inline
    /// trivial). Extremely Cursed.
    detail::function::abstract_wrapper* m_wrapper{nullptr};
};

static_assert(sizeof(function<void()>) == 32);

template <typename T>
using copyable_function = function<T, true>;

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
    swap(a, b);
}

/// A fairly unsafe reference to a functor. Efficient if function pointer, dangerous if capturing
/// lambda.
template <typename Fn>
class function_view;

template <typename Out, typename... In>
class function_view<Out(In...)> {
    using FunctionPointer = Out (*)(In...);

public:
    template <typename Fn>
    explicit function_view(Fn& fn) : m_erased_ptr{&fn} {
        m_invoker = [](void* ptr, In... args) -> Out { return (*reinterpret_cast<Fn*>(ptr))(args...); };
    }

    function_view(Out (*f_ptr)(In...)) : m_erased_ptr{f_ptr} {  // NOLINT(*-explicit-constructor)
        m_invoker = [](void* ptr, In&&... args) -> Out {
            return (*reinterpret_cast<FunctionPointer>(ptr))(static_cast<decltype(args)>(args)...);
        };
    }

    decltype(auto) operator()(In... args) const noexcept { return m_invoker(m_erased_ptr, bek::forward<In>(args)...); }

private:
    using InvokerPtr = Out (*)(void*, In...);
    void* m_erased_ptr;
    InvokerPtr m_invoker;
};

}  // namespace bek
#endif  // BEKOS_FUNCTION_H
