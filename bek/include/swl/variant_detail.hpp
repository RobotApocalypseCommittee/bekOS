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



#ifdef SWL_CPP_LIBRARY_VARIANT_HPP

template <int N>
constexpr int find_first_true(bool (&&arr)[N]) {
    for (int k = 0; k < N; ++k)
        if (arr[k]) return k;
    return -1;
}

template <class T, class... Ts>
inline constexpr bool appears_exactly_once =
    (static_cast<unsigned short>(bek::is_same<T, Ts>) + ...) == 1;

// ============= type pack element

template <unsigned char = 1>
struct find_type_i;

template <>
struct find_type_i<1> {
    template <uSize Idx, class T, class... Ts>
    using f = typename find_type_i<(Idx != 1)>::template f<Idx - 1, Ts...>;
};

template <>
struct find_type_i<0> {
    template <uSize, class T, class... Ts>
    using f = T;
};

template <uSize K, class... Ts>
using type_pack_element = typename find_type_i<(K != 0)>::template f<K, Ts...>;

// ============= overload match detector. to be used for variant generic assignment

template <class T>
using arr1 = T[1];

template <uSize N, class A>
struct overload_frag {
    using type = A;
    template <class T>
        requires requires { arr1<A>{bek::declval<T>()}; }
    auto operator()(A, T&&) -> overload_frag<N, A>;
};

template <class Seq, class... Args>
struct make_overload;

template <uSize... Idx, class... Args>
struct make_overload<bek::integer_sequence<uSize, Idx...>, Args...> : overload_frag<Idx, Args>... {
    using overload_frag<Idx, Args>::operator()...;
};

template <class T, class... Ts>
using best_overload_match =
    typename decltype(make_overload<bek::make_index_sequence<sizeof...(Ts)>, Ts...>{}(
        bek::declval<T>(), bek::declval<T>()))::type;

template <class T, class... Ts>
concept has_non_ambiguous_match = requires { typename best_overload_match<T, Ts...>; };

// ================================== rel ops

template <class From, class To>
concept convertible = bek::implictly_convertible_to<From, To>;

template <class T>
concept has_eq_comp = requires(T a, T b) {
    { a == b } -> convertible<bool>;
};

template <class T>
concept has_lesser_comp = requires(T a, T b) {
    { a < b } -> convertible<bool>;
};

template <class T>
concept has_less_or_eq_comp = requires(T a, T b) {
    { a <= b } -> convertible<bool>;
};

template <class A>
struct emplace_no_dtor_from_elem {
    template <class T>
    constexpr void operator()(T&& elem, auto index_) const {
        a.template emplace_no_dtor<index_>(static_cast<T&&>(elem));
    }
    A& a;
};

template <class E, class T>
constexpr void destruct(T& obj) {
    if constexpr (not bek::is_trivially_destructible<E>) obj.~E();
}

// =============================== variant union types

// =================== base variant storage type
// this type is used to build a N-ary tree of union.

struct dummy_type {
    static constexpr int elem_size = 0;
};  // used to fill the back of union nodes

using union_index_t = unsigned;

#define TRAIT(trait) (bek::trait<A> && bek::trait<B>)

#define SFM(signature, trait)                                   \
    signature = default;                                        \
    signature                                                   \
        requires(TRAIT(trait) and not TRAIT(trivially_##trait)) \
    {}

// given the two members of type A and B of an union X
// this create the proper conditionally trivial special members functions
#define INJECT_UNION_SFM(X)                                \
    SFM(constexpr X(const X&), copy_constructible)         \
    SFM(constexpr X(X&&), move_constructible)              \
    SFM(constexpr X& operator=(const X&), copy_assignable) \
    SFM(constexpr X& operator=(X&&), move_assignable)      \
    SFM(constexpr ~X(), destructible)

template <bool IsLeaf>
struct node_trait;

template <>
struct node_trait<true> {
    template <class A, class B>
    static constexpr auto elem_size = not(bek::is_same<B, dummy_type>) ? 2 : 1;

    template <uSize, class>
    static constexpr char ctor_branch = 0;
};

template <>
struct node_trait<false> {
    template <class A, class B>
    static constexpr auto elem_size = A::elem_size + B::elem_size;

    template <uSize Index, class A>
    static constexpr char ctor_branch = (Index < A::elem_size) ? 1 : 2;
};

template <bool IsLeaf, class A, class B>
struct union_node {
    union {
        A a;
        B b;
    };

    static constexpr auto elem_size = node_trait<IsLeaf>::template elem_size<A, B>;

    constexpr union_node() = default;

    template <uSize Index, class... Args>
        requires(node_trait<IsLeaf>::template ctor_branch<Index, A> == 1)
    constexpr union_node(in_place_index_t<Index>, Args&&... args)
        : a{in_place_index<Index>, static_cast<Args&&>(args)...} {}

    template <uSize Index, class... Args>
        requires(node_trait<IsLeaf>::template ctor_branch<Index, A> == 2)
    constexpr union_node(in_place_index_t<Index>, Args&&... args)
        : b{in_place_index<Index - A::elem_size>, static_cast<Args&&>(args)...} {}

    template <class... Args>
        requires(IsLeaf)
    constexpr union_node(in_place_index_t<0>, Args&&... args) : a{static_cast<Args&&>(args)...} {}

    template <class... Args>
        requires(IsLeaf)
    constexpr union_node(in_place_index_t<1>, Args&&... args) : b{static_cast<Args&&>(args)...} {}

    constexpr union_node(dummy_type)
        requires(bek::is_same<dummy_type, B>)
        : b{} {}

    template <union_index_t Index>
    constexpr auto& get() {
        if constexpr (IsLeaf) {
            if constexpr (Index == 0)
                return a;
            else
                return b;
        } else {
            if constexpr (Index < A::elem_size)
                return a.template get<Index>();
            else
                return b.template get<Index - A::elem_size>();
        }
    }

    INJECT_UNION_SFM(union_node)
};

#undef INJECT_UNION_SFM
#undef SFM
#undef TRAIT

// =================== algorithm to build the tree of unions
// take a sequence of types and perform an order preserving fold until only one type is left
// the first parameter is the numbers of types remaining for the current pass

constexpr unsigned char pick_next(unsigned remaining) { return remaining >= 2 ? 2 : remaining; }

template <unsigned char Pick, unsigned char GoOn, bool FirstPass>
struct make_tree;

template <bool IsFirstPass>
struct make_tree<2, 1, IsFirstPass> {
    template <unsigned Remaining, class A, class B, class... Ts>
    using f =
        typename make_tree<pick_next(Remaining - 2), sizeof...(Ts) != 0, IsFirstPass>::template f<
            Remaining - 2, Ts..., union_node<IsFirstPass, A, B>>;
};

// only one type left, stop
template <bool F>
struct make_tree<0, 0, F> {
    template <unsigned, class A>
    using f = A;
};

// end of one pass, restart
template <bool IsFirstPass>
struct make_tree<0, 1, IsFirstPass> {
    template <unsigned Remaining, class... Ts>
    using f = typename make_tree<pick_next(sizeof...(Ts)), (sizeof...(Ts) != 1),
                                 false  // <- both first pass and tail call recurse into a tail call
                                 >::template f<sizeof...(Ts), Ts...>;
};

// one odd type left in the pass, put it at the back to preserve the order
template <>
struct make_tree<1, 1, false> {
    template <unsigned Remaining, class A, class... Ts>
    using f = typename make_tree<0, sizeof...(Ts) != 0, false>::template f<0, Ts..., A>;
};

// one odd type left in the first pass, wrap it in an union
template <>
struct make_tree<1, 1, true> {
    template <unsigned, class A, class... Ts>
    using f = typename make_tree<0, sizeof...(Ts) != 0,
                                 false>::template f<0, Ts..., union_node<true, A, dummy_type>>;
};

template <class... Ts>
using make_tree_union =
    typename make_tree<pick_next(sizeof...(Ts)), 1, true>::template f<sizeof...(Ts), Ts...>;

// ============================================================

// Ts... must be sorted in ascending size
template <uSize Num, class... Ts>
using smallest_suitable_integer_type =
    type_pack_element<(static_cast<unsigned char>(Num > bek::numeric_limits<Ts>::max()) + ...),
                      Ts...>;

// why do we need this again? i think something to do with GCC?
namespace swap_trait {
using bek::swap;

template <class A>
concept able = requires(A a, A b) { swap(a, b); };

template <class A>
inline constexpr bool nothrow = noexcept(swap(bek::declval<A&>(), bek::declval<A&>()));
}  // namespace swap_trait

template <class T>
inline constexpr T* addressof(T& obj) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_addressof(obj);
#elif defined(SWL_VARIANT_NO_CONSTEXPR_EMPLACE)
    // if & is overloaded, use the ugly version
    if constexpr (requires { obj.operator&(); })
        return reinterpret_cast<T*>(
            &const_cast<char&>(reinterpret_cast<const volatile char&>(obj)));
    else
        return &obj;
#else
    return std::address_of(obj);
#endif
}

#endif  // eof
