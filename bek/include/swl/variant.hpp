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

#ifndef SWL_CPP_LIBRARY_VARIANT_HPP
#define SWL_CPP_LIBRARY_VARIANT_HPP

// #include <exception>
// #include <type_traits>
// #include <utility> // swap
// #include <limits> // used for index_type
#include <bek/initializer_list.h>

#include "bek/assertions.h"
#include "bek/numeric_limits.h"
#include "bek/traits.h"
#include "bek/types.h"
#include "bek/utility.h"

#define SWL_FWD(x) static_cast<decltype(x)&&>(x)
#define SWL_MOV(x) static_cast<bek::remove_reference<decltype(x)>&&>(x)

#define DebugAssert(X) ASSERT(X)

namespace swl {

namespace vimpl {
struct variant_tag {};
struct emplacer_tag {};
}  // namespace vimpl

template <class T>
struct in_place_type_t : private vimpl::emplacer_tag {};

template <uSize Index>
struct in_place_index_t : private vimpl::emplacer_tag {};

template <uSize Index>
inline static constexpr in_place_index_t<Index> in_place_index;

template <class T>
inline static constexpr in_place_type_t<T> in_place_type;

namespace vimpl {
#include "variant_detail.hpp"
#include "variant_visit.hpp"

struct variant_npos_t {
    template <class T>
    constexpr bool operator==(T idx) const noexcept {
        return idx == bek::numeric_limits<T>::max();
    }
};
}  // namespace vimpl

template <class T>
inline constexpr bool is_variant = bek::is_base_of<vimpl::variant_tag, bek::remove_cv_reference<T>>;

inline static constexpr vimpl::variant_npos_t variant_npos;

template <class... Ts>
class variant;

// ill-formed variant, an empty specialization prevents some really bad errors messages on gcc
template <class... Ts>
    requires((bek::is_array<Ts> || ...) || (bek::is_reference<Ts> || ...) ||
             (bek::is_same<void, bek::remove_cv<Ts>> || ...) || sizeof...(Ts) == 0)
class variant<Ts...> {
    static_assert(sizeof...(Ts) > 0, "A variant cannot be empty.");
    static_assert(
        not(bek::is_reference<Ts> || ...),
        "A variant cannot contain references, consider using reference wrappers instead.");
    static_assert(not(bek::is_same<void, bek::remove_cv<Ts>> || ...),
                  "A variant cannot contains void.");
    static_assert(not(bek::is_array<Ts> || ...),
                  "A variant cannot contain a raw array type, consider using std::array instead.");
};

template <class... Ts>
class variant : private vimpl::variant_tag {
    using storage_t = vimpl::union_node<false, vimpl::make_tree_union<Ts...>, vimpl::dummy_type>;

    static constexpr bool is_trivial    = bek::is_trivial<storage_t>;
    static constexpr bool has_copy_ctor = bek::copy_constructible<storage_t>;
    static constexpr bool trivial_copy_ctor =
        is_trivial || bek::is_trivially_copy_constructible<storage_t>;
    static constexpr bool has_copy_assign = bek::copy_constructible<storage_t>;
    static constexpr bool trivial_copy_assign =
        is_trivial || bek::is_trivially_copy_assignable<storage_t>;
    static constexpr bool has_move_ctor = bek::move_constructible<storage_t>;
    static constexpr bool trivial_move_ctor =
        is_trivial || bek::is_trivially_move_constructible<storage_t>;
    static constexpr bool has_move_assign = bek::move_assignable<storage_t>;
    static constexpr bool trivial_move_assign =
        is_trivial || bek::is_trivially_move_assignable<storage_t>;
    static constexpr bool trivial_dtor = bek::is_trivially_destructible<storage_t>;

public:
    template <uSize Idx>
    using alternative =
        bek::remove_reference<decltype(bek::declval<storage_t&>().template get<Idx>())>;

    static constexpr bool can_be_valueless = not is_trivial;

    static constexpr unsigned size = sizeof...(Ts);

    using index_type =
        vimpl::smallest_suitable_integer_type<sizeof...(Ts) + can_be_valueless, unsigned char,
                                              unsigned short, unsigned>;

    static constexpr index_type npos = -1;

    template <class T>
    static constexpr int index_of = vimpl::find_first_true({bek::is_same<T, Ts>...});

    // ============================================= constructors (20.7.3.2)

    // default constructor
    constexpr variant() noexcept
        requires bek::default_constructible<alternative<0>>
        : storage{in_place_index<0>}, current{0} {}

    // copy constructor (trivial)
    constexpr variant(const variant&)
        requires trivial_copy_ctor
    = default;

    // note : both the copy and move constructor cannot be meaningfully constexpr without
    // std::construct_at copy constructor
    constexpr variant(const variant& o)
        requires(has_copy_ctor and not trivial_copy_ctor)
        : storage{vimpl::dummy_type{}} {
        construct_from(o);
    }

    // move constructor (trivial)
    constexpr variant(variant&&)
        requires trivial_move_ctor
    = default;

    // move constructor
    constexpr variant(variant&& o) noexcept
        requires(has_move_ctor and not trivial_move_ctor)
        : storage{vimpl::dummy_type{}} {
        construct_from(static_cast<variant&&>(o));
    }

    // generic constructor
    template <class T, class M = vimpl::best_overload_match<T&&, Ts...>,
              class D = bek::remove_cv_reference<T>>
    constexpr variant(T&& t) noexcept
        requires(not bek::is_same<D, variant> and not bek::is_base_of<vimpl::emplacer_tag, D>)
        : variant{in_place_index<index_of<M>>, static_cast<T&&>(t)} {}

    // construct at index
    template <uSize Index, class... Args>
        requires(Index < size && bek::constructible<alternative<Index>, Args && ...>)
    explicit constexpr variant(in_place_index_t<Index> tag, Args&&... args)
        : storage{tag, static_cast<Args&&>(args)...}, current(Index) {}

    // construct a given type
    template <class T, class... Args>
        requires(vimpl::appears_exactly_once<T, Ts...> && bek::constructible<T, Args && ...>)
    explicit constexpr variant(in_place_type_t<T>, Args&&... args)
        : variant{in_place_index<index_of<T>>, static_cast<Args&&>(args)...} {}

    // initializer-list constructors
    template <uSize Index, class U, class... Args>
        requires((Index < size) and
                 bek::constructible<alternative<Index>, std::initializer_list<U>&, Args && ...>)
    explicit constexpr variant(in_place_index_t<Index> tag, std::initializer_list<U> list,
                               Args&&... args)
        : storage{tag, list, SWL_FWD(args)...}, current{Index} {}

    template <class T, class U, class... Args>
        requires(vimpl::appears_exactly_once<T, Ts...> &&
                 bek::constructible<T, std::initializer_list<U>&, Args && ...>)
    explicit constexpr variant(in_place_type_t<T>, std::initializer_list<U> list, Args&&... args)
        : storage{in_place_index<index_of<T>>, list, SWL_FWD(args)...}, current{index_of<T>} {}

    // ================================ destructors (20.7.3.3)

    constexpr ~variant()
        requires trivial_dtor
    = default;

    constexpr ~variant()
        requires(not trivial_dtor)
    {
        reset();
    }

    // ================================ assignment (20.7.3.4)

    // copy assignment (trivial)
    constexpr variant& operator=(const variant& o)
        requires trivial_copy_assign&& trivial_copy_ctor
    = default;

    // copy assignment
    constexpr variant& operator=(const variant& rhs)
        requires(has_copy_assign and not(trivial_copy_assign && trivial_copy_ctor))
    {
        assign_from(rhs, [this](const auto& elem, auto index_cst) {
            if (index() == index_cst)
                unsafe_get<index_cst>() = elem;
            else {
                using type = alternative<index_cst>;
                // TODO: Originally both of these conditions were nothrow - why?
                if constexpr (bek::is_copy_constructible<type> or
                              not bek::is_move_constructible<type>)
                    this->emplace<index_cst>(elem);
                else {
                    alternative<index_cst> tmp = elem;
                    this->emplace<index_cst>(SWL_MOV(tmp));
                }
            }
        });
        return *this;
    }

    // move assignment (trivial)
    constexpr variant& operator=(variant&& o)
        requires(trivial_move_assign and trivial_move_ctor and trivial_dtor)
    = default;

    // move assignment
    constexpr variant& operator=(variant&& o) noexcept
        requires(has_move_assign && has_move_ctor and
                 not(trivial_move_assign and trivial_move_ctor and trivial_dtor))
    {
        this->assign_from(SWL_FWD(o), [this](auto&& elem, auto index_cst) {
            if (index() == index_cst)
                this->unsafe_get<index_cst>() = SWL_MOV(elem);
            else
                this->emplace<index_cst>(SWL_MOV(elem));
        });
        return *this;
    }

    // generic assignment
    template <class T>
        requires vimpl::has_non_ambiguous_match<T, Ts...>
    constexpr variant& operator=(T&& t) noexcept {
        using related_type       = vimpl::best_overload_match<T&&, Ts...>;
        constexpr auto new_index = index_of<related_type>;

        if (this->current == new_index)
            this->unsafe_get<new_index>() = SWL_FWD(t);
        else {
            constexpr bool do_simple_emplace =
                bek::constructible<related_type, T> or not bek::is_move_constructible<related_type>;

            if constexpr (do_simple_emplace)
                this->emplace<new_index>(SWL_FWD(t));
            else {
                related_type tmp = t;
                this->emplace<new_index>(SWL_MOV(tmp));
            }
        }

        return *this;
    }

    // ================================== modifiers (20.7.3.5)

    template <class T, class... Args>
        requires(bek::constructible<T, Args && ...> && vimpl::appears_exactly_once<T, Ts...>)
    constexpr T& emplace(Args&&... args) {
        return emplace<index_of<T>>(static_cast<Args&&>(args)...);
    }

    template <uSize Idx, class... Args>
        requires(Idx < size and bek::constructible<alternative<Idx>, Args && ...>)
    constexpr auto& emplace(Args&&... args) {
        return emplace_impl<Idx>(SWL_FWD(args)...);
    }

    // emplace with initializer-lists
    template <uSize Idx, class U, class... Args>
        requires(Idx < size &&
                 bek::constructible<alternative<Idx>, std::initializer_list<U>&, Args && ...>)
    constexpr auto& emplace(std::initializer_list<U> list, Args&&... args) {
        return emplace_impl<Idx>(list, SWL_FWD(args)...);
    }

    template <class T, class U, class... Args>
        requires(bek::constructible<T, std::initializer_list<U>&, Args && ...> &&
                 vimpl::appears_exactly_once<T, Ts...>)
    constexpr T& emplace(std::initializer_list<U> list, Args&&... args) {
        return emplace_impl<index_of<T>>(list, SWL_FWD(args)...);
    }

    // ==================================== value status (20.7.3.6)

    constexpr bool valueless_by_exception() const noexcept {
        if constexpr (can_be_valueless)
            return current == npos;
        else
            return false;
    }

    constexpr index_type index() const noexcept { return current; }

    // =================================== swap (20.7.3.7)

    constexpr void swap(variant& o) noexcept((bek::is_nothrow_move_constructible<Ts> && ...) &&
                                             (vimpl::swap_trait::template nothrow<Ts> && ...))
        requires(has_move_ctor && (vimpl::swap_trait::template able<Ts> && ...))
    {
        if constexpr (can_be_valueless) {
            // if one is valueless, move the element form the non-empty variant,
            // reset it, and set it to valueless
            constexpr auto impl_one_valueless = [](auto&& full, auto& empty) {
                vimpl::visit_with_index(SWL_FWD(full),
                                        vimpl::emplace_no_dtor_from_elem<variant&>{empty});
                full.reset_no_check();
                full.current = npos;
            };

            switch (static_cast<int>(this->index() == npos) +
                    static_cast<int>(o.index() == npos) * 2) {
                case 0:
                    break;
                case 1:
                    // "this" is valueless
                    impl_one_valueless(SWL_MOV(o), *this);
                    return;
                case 2:
                    // "other" is valueless
                    impl_one_valueless(SWL_MOV(*this), o);
                    return;
                case 3:
                    // both are valueless, do nothing
                    return;
            }
        }

        DebugAssert(not(valueless_by_exception() && o.valueless_by_exception()));

        vimpl::visit_with_index(o, [&o, this](auto&& elem, auto index_) {
            if (this->index() == index_) {
                using bek::swap;
                swap(this->unsafe_get<index_>(), elem);
                return;
            }

            using idx_t = decltype(index_);
            vimpl::visit_with_index(*this, [this, &o, &elem](auto&& this_elem, auto this_index) {
                auto tmp{SWL_MOV(this_elem)};

                // destruct the element
                vimpl::destruct<alternative<this_index>>(this_elem);

                // ok, we just destroyed the element in this, don't call the dtor again
                this->emplace_no_dtor<idx_t::value>(SWL_MOV(elem));

                // we could refactor this
                vimpl::destruct<alternative<idx_t::value>>(elem);
                o.template emplace_no_dtor<(unsigned)(this_index)>(SWL_MOV(tmp));
            });
        });
    }

    // +================================== methods for internal use
    // these methods performs no errors checking at all

    template <vimpl::union_index_t Idx>
    constexpr auto& unsafe_get() & noexcept {
        static_assert(Idx < size);
        DebugAssert(current == Idx);
        return storage.template get<Idx>();
    }

    template <vimpl::union_index_t Idx>
    constexpr auto&& unsafe_get() && noexcept {
        static_assert(Idx < size);
        DebugAssert(current == Idx);
        return SWL_MOV(storage.template get<Idx>());
    }

    template <vimpl::union_index_t Idx>
    constexpr const auto& unsafe_get() const& noexcept {
        static_assert(Idx < size);
        DebugAssert(current == Idx);
        return const_cast<variant&>(*this).unsafe_get<Idx>();
    }

    template <vimpl::union_index_t Idx>
    constexpr const auto&& unsafe_get() const&& noexcept {
        static_assert(Idx < size);
        DebugAssert(current == Idx);
        return SWL_MOV(unsafe_get<Idx>());
    }

private:
    // assign from another variant
    template <class Other, class Fn>
    constexpr void assign_from(Other&& o, Fn&& fn) {
        if constexpr (can_be_valueless) {
            if (o.index() == npos) {
                if (current != npos) {
                    reset_no_check();
                    current = npos;
                }
                return;
            }
        }
        DebugAssert(not o.valueless_by_exception());
        vimpl::visit_with_index(SWL_FWD(o), SWL_FWD(fn));
    }

    template <unsigned Idx, class... Args>
    constexpr auto& emplace_impl(Args&&... args) {
        reset();
        emplace_no_dtor<Idx>(SWL_FWD(args)...);
        return unsafe_get<Idx>();
    }

    // can be used directly only when the variant is in a known empty state
    template <unsigned Idx, class... Args>
    constexpr void emplace_no_dtor(Args&&... args) {
        using T = alternative<Idx>;

        if constexpr (not bek::constructible<T, Args&&...>) {
            if constexpr (bek::is_nothrow_move_constructible<T>)
                do_emplace_no_dtor<Idx>(T{SWL_FWD(args)...});
            else if constexpr (bek::is_nothrow_copy_constructible<T>) {
                T tmp{SWL_FWD(args)...};
                do_emplace_no_dtor<Idx>(tmp);
            } else {
                static_assert(can_be_valueless && (Idx == Idx),
                              "Internal error : the possibly valueless branch of emplace was taken "
                              "despite |can_be_valueless| being false");
                current = npos;
                do_emplace_no_dtor<Idx>(SWL_FWD(args)...);
            }
        } else
            do_emplace_no_dtor<Idx>(SWL_FWD(args)...);
    }

    template <unsigned Idx, class... Args>
    constexpr void do_emplace_no_dtor(Args&&... args) {
        auto* ptr = vimpl::addressof(unsafe_get<Idx>());

        using T = alternative<Idx>;
        new ((void*)(ptr)) T(SWL_FWD(args)...);

        current = static_cast<index_type>(Idx);
    }

    // destroy the current elem IFF not valueless
    constexpr void reset() {
        if constexpr (can_be_valueless)
            if (valueless_by_exception()) return;
        reset_no_check();
    }

    // destroy the current element without checking for valueless
    constexpr void reset_no_check() {
        DebugAssert(index() < size);
        if constexpr (not trivial_dtor) {
            vimpl::visit_with_index(
                *this, [](auto& elem, auto index_) { vimpl::destruct<alternative<index_>>(elem); });
        }
    }

    // construct this from another variant, for constructors only
    template <class Other>
    constexpr void construct_from(Other&& o) {
        if constexpr (can_be_valueless)
            if (o.valueless_by_exception()) {
                current = npos;
                return;
            }

        vimpl::visit_with_index(SWL_FWD(o), vimpl::emplace_no_dtor_from_elem<variant&>{*this});
    }

    template <class T>
    friend struct vimpl::emplace_no_dtor_from_elem;

    storage_t storage;
    index_type current;
};

// ================================= value access (20.7.5)

template <class T, class... Ts>
constexpr bool holds_alternative(const variant<Ts...>& v) noexcept {
    static_assert((bek::is_same<T, Ts> || ...), "Requested type is not contained in the variant");
    constexpr auto Index = variant<Ts...>::template index_of<T>;
    return v.index() == Index;
}

// ========= get by index

template <uSize Idx, class... Ts>
constexpr auto& get(variant<Ts...>& v) {
    static_assert(Idx < sizeof...(Ts), "Index exceeds the variant size. ");
    ASSERT(v.index() == Idx && "swl::variant : Bad variant access in get.");
    return (v.template unsafe_get<Idx>());
}

template <uSize Idx, class... Ts>
constexpr const auto& get(const variant<Ts...>& v) {
    return swl::get<Idx>(const_cast<variant<Ts...>&>(v));
}

template <uSize Idx, class... Ts>
constexpr auto&& get(variant<Ts...>&& v) {
    return SWL_MOV(swl::get<Idx>(v));
}

template <uSize Idx, class... Ts>
constexpr const auto&& get(const variant<Ts...>&& v) {
    return SWL_MOV(swl::get<Idx>(v));
}

// ========= get by type

template <class T, class... Ts>
constexpr T& get(variant<Ts...>& v) {
    return swl::get<variant<Ts...>::template index_of<T>>(v);
}

template <class T, class... Ts>
constexpr const T& get(const variant<Ts...>& v) {
    return swl::get<variant<Ts...>::template index_of<T>>(v);
}

template <class T, class... Ts>
constexpr T&& get(variant<Ts...>&& v) {
    return swl::get<variant<Ts...>::template index_of<T>>(SWL_FWD(v));
}

template <class T, class... Ts>
constexpr const T&& get(const variant<Ts...>&& v) {
    return swl::get<variant<Ts...>::template index_of<T>>(SWL_FWD(v));
}

// ===== get_if by index

template <uSize Idx, class... Ts>
constexpr const auto* get_if(const variant<Ts...>* v) noexcept {
    using rtype = typename variant<Ts...>::template alternative<Idx>*;
    if (v == nullptr || v->index() != Idx)
        return rtype{nullptr};
    else
        return vimpl::addressof(v->template unsafe_get<Idx>());
}

template <uSize Idx, class... Ts>
constexpr auto* get_if(variant<Ts...>* v) noexcept {
    using rtype = typename variant<Ts...>::template alternative<Idx>;
    return const_cast<rtype*>(swl::get_if<Idx>(static_cast<const variant<Ts...>*>(v)));
}

// ====== get_if by type

template <class T, class... Ts>
constexpr T* get_if(variant<Ts...>* v) noexcept {
    static_assert((bek::is_same<T, Ts> || ...), "Requested type is not contained in the variant");
    return swl::get_if<variant<Ts...>::template index_of<T>>(v);
}

template <class T, class... Ts>
constexpr const T* get_if(const variant<Ts...>* v) noexcept {
    static_assert((bek::is_same<T, Ts> || ...), "Requested type is not contained in the variant");
    return swl::get_if<variant<Ts...>::template index_of<T>>(v);
}

// =============================== visitation (20.7.7)

template <class Fn, class... Vs>
constexpr decltype(auto) visit(Fn&& fn, Vs&&... vs) {
    if constexpr ((bek::remove_cv_reference<Vs>::can_be_valueless || ...))
        ASSERT(!(vs.valueless_by_exception() || ...) &&
               "swl::variant : Bad variant access in visit.");

    if constexpr (sizeof...(Vs) == 1)
        return vimpl::visit(SWL_FWD(fn), SWL_FWD(vs)...);
    else
        return vimpl::multi_visit(SWL_FWD(fn), SWL_FWD(vs)...);
}

template <class Fn>
constexpr decltype(auto) visit(Fn&& fn) {
    return SWL_FWD(fn)();
}

template <class R, class Fn, class... Vs>
    requires(is_variant<Vs> && ...)
constexpr R visit(Fn&& fn, Vs&&... vars) {
    return static_cast<R>(swl::visit(SWL_FWD(fn), SWL_FWD(vars)...));
}

// ============================== relational operators (20.7.6)

template <class... Ts>
    requires(vimpl::has_eq_comp<Ts> && ...)
constexpr bool operator==(const variant<Ts...>& v1, const variant<Ts...>& v2) {
    if (v1.index() != v2.index()) return false;
    if constexpr (variant<Ts...>::can_be_valueless)
        if (v1.valueless_by_exception()) return true;
    return vimpl::visit_with_index(v2, [&v1](auto& elem, auto index) -> bool {
        return (v1.template unsafe_get<index>() == elem);
    });
}

template <class... Ts>
constexpr bool operator!=(const variant<Ts...>& v1, const variant<Ts...>& v2)
    requires requires { v1 == v2; }
{
    return not(v1 == v2);
}

template <class... Ts>
    requires(vimpl::has_lesser_comp<const Ts&> && ...)
constexpr bool operator<(const variant<Ts...>& v1, const variant<Ts...>& v2) {
    if constexpr (variant<Ts...>::can_be_valueless) {
        if (v2.valueless_by_exception()) return false;
        if (v1.valueless_by_exception()) return true;
    }
    if (v1.index() == v2.index()) {
        return vimpl::visit_with_index(v1, [&v2](auto& elem, auto index) -> bool {
            return (elem < v2.template unsafe_get<index>());
        });
    } else
        return (v1.index() < v2.index());
}

template <class... Ts>
constexpr bool operator>(const variant<Ts...>& v1, const variant<Ts...>& v2)
    requires requires { v2 < v1; }
{
    return v2 < v1;
}

template <class... Ts>
    requires(vimpl::has_less_or_eq_comp<const Ts&> && ...)
constexpr bool operator<=(const variant<Ts...>& v1, const variant<Ts...>& v2) {
    if constexpr (variant<Ts...>::can_be_valueless) {
        if (v1.valueless_by_exception()) return true;
        if (v2.valueless_by_exception()) return false;
    }
    if (v1.index() == v2.index()) {
        return vimpl::visit_with_index(v1, [&v2](auto& elem, auto index) -> bool {
            return (elem <= v2.template unsafe_get<index>());
        });
    } else
        return (v1.index() < v2.index());
}

template <class... Ts>
constexpr bool operator>=(const variant<Ts...>& v1, const variant<Ts...>& v2)
    requires requires { v2 <= v1; }
{
    return v2 <= v1;
}

// ===================================== monostate (20.7.8, 20.7.9)

struct monostate {};
constexpr bool operator==(monostate, monostate) noexcept { return true; }
constexpr bool operator>(monostate, monostate) noexcept { return false; }
constexpr bool operator<(monostate, monostate) noexcept { return false; }
constexpr bool operator<=(monostate, monostate) noexcept { return true; }
constexpr bool operator>=(monostate, monostate) noexcept { return true; }

// ===================================== specialized algorithms (20.7.10)

template <class... Ts>
constexpr void swap(variant<Ts...>& a, variant<Ts...>& b) noexcept(noexcept(a.swap(b)))
    requires requires { a.swap(b); }
{
    a.swap(b);
}

// ===================================== helper classes (20.7.4)

template <class T>
    requires is_variant<T>
inline constexpr uSize variant_size_v = bek::remove_cv_reference<T>::size;

// not sure why anyone would need this, i'm adding it anyway
template <class T>
    requires is_variant<T>
struct variant_size : bek::integral_constant<uSize, variant_size_v<T>> {};

namespace vimpl {
// ugh, we have to take care of volatile here
template <bool IsVolatile>
struct var_alt_impl {
    template <uSize Idx, class T>
    using type = bek::remove_reference<decltype(bek::declval<T>().template unsafe_get<Idx>())>;
};

template <>
struct var_alt_impl<true> {
    template <uSize Idx, class T>
    using type = volatile typename var_alt_impl<false>::template type<Idx, bek::remove_volatile<T>>;
};
}  // namespace vimpl

template <uSize Idx, class T>
    requires(Idx < variant_size_v<T>)
using variant_alternative_t =
    typename vimpl::var_alt_impl<bek::is_volatile<T>>::template type<Idx, T>;

template <uSize Idx, class T>
    requires is_variant<T>
struct variant_alternative {
    using type = variant_alternative_t<Idx, T>;
};

// ===================================== extensions (unsafe_get)

template <uSize Idx, class Var>
    requires is_variant<Var>
constexpr auto&& unsafe_get(Var&& var) noexcept {
    static_assert(Idx < bek::remove_cv_reference<Var>::size, "Index exceeds the variant size.");
    return SWL_FWD(var).template unsafe_get<Idx>();
}

template <class T, class Var>
    requires is_variant<Var>
constexpr auto&& unsafe_get(Var&& var) noexcept {
    return swl::unsafe_get<bek::remove_cv_reference<Var>::template index_of<T>>(SWL_FWD(var));
}

}  // namespace swl

#undef DebugAssert
#undef SWL_FWD
#undef SWL_MOV

#endif  // eof