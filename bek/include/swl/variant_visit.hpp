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

// ========================= visit dispatcher

template <class Fn, class... Vars>
using rtype_visit =
    decltype((bek::declval<Fn>()(bek::declval<Vars>().template unsafe_get<0>()...)));

template <class Fn, class Var>
using rtype_index_visit = decltype((bek::declval<Fn>()(bek::declval<Var>().template unsafe_get<0>(),
                                                       bek::integral_constant<uSize, 0>{})));

inline namespace v1 {

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define DeclareUnreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
#define DeclareUnreachable() __assume(false)
#else
#error "Compiler not supported, please file an issue."
#endif

#define DEC(N)                                                                              \
    X((N))                                                                                  \
    X((N) + 1) X((N) + 2) X((N) + 3) X((N) + 4) X((N) + 5) X((N) + 6) X((N) + 7) X((N) + 8) \
        X((N) + 9)

#define SEQ30(N) DEC((N) + 0) DEC((N) + 10) DEC((N) + 20)
#define SEQ100(N) SEQ30((N) + 0) SEQ30((N) + 30) SEQ30((N) + 60) DEC((N) + 90)
#define SEQ200(N) SEQ100((N) + 0) SEQ100((N) + 100)
#define SEQ400(N) SEQ200((N) + 0) SEQ200((N) + 200)
#define CAT(M, N) M##N
#define CAT2(M, N) CAT(M, N)
#define INJECTSEQ(N) CAT2(SEQ, N)(0)

// single-visitation

template <unsigned Offset, class Rtype, class Fn, class V>
constexpr Rtype single_visit_tail(Fn&& fn, V&& v) {
    constexpr auto RemainingIndex = bek::remove_cv_reference<V>::size - Offset;

#define X(N)                                                                                     \
    case (N + Offset):                                                                           \
        if constexpr (N < RemainingIndex) {                                                      \
            return static_cast<Fn&&>(fn)(static_cast<V&&>(v).template unsafe_get<N + Offset>()); \
            break;                                                                               \
        } else                                                                                   \
            DeclareUnreachable();

#define SEQSIZE 200

    switch (v.index()) {
        INJECTSEQ(SEQSIZE)

        default:
            if constexpr (SEQSIZE < RemainingIndex)
                return vimpl::single_visit_tail<Offset + SEQSIZE, Rtype>(static_cast<Fn&&>(fn),
                                                                         static_cast<V&&>(v));
            else
                DeclareUnreachable();
    }

#undef X
#undef SEQSIZE
}

template <unsigned Offset, class Rtype, class Fn, class V>
constexpr Rtype single_visit_w_index_tail(Fn&& fn, V&& v) {
    constexpr auto RemainingIndex = bek::remove_cv_reference<V>::size - Offset;

#define X(N)                                                                                    \
    case (N + Offset):                                                                          \
        if constexpr (N < RemainingIndex) {                                                     \
            return static_cast<Fn&&>(fn)(static_cast<V&&>(v).template unsafe_get<N + Offset>(), \
                                         bek::integral_constant<unsigned, N + Offset>{});       \
            break;                                                                              \
        } else                                                                                  \
            DeclareUnreachable();

#define SEQSIZE 200

    switch (v.index()) {
        INJECTSEQ(SEQSIZE)

        default:
            if constexpr (SEQSIZE < RemainingIndex)
                return vimpl::single_visit_w_index_tail<Offset + SEQSIZE, Rtype>(
                    static_cast<Fn&&>(fn), static_cast<V&&>(v));
            else
                DeclareUnreachable();
    }

#undef X
#undef SEQSIZE
}

template <class Fn, class V>
constexpr decltype(auto) visit(Fn&& fn, V&& v) {
    return vimpl::single_visit_tail<0, rtype_visit<Fn&&, V&&>>(SWL_FWD(fn), SWL_FWD(v));
}

// unlike other visit functions, this takes the variant first!
// this is confusing, but make the client code easier to read
template <class Fn, class V>
constexpr decltype(auto) visit_with_index(V&& v, Fn&& fn) {
    return vimpl::single_visit_w_index_tail<0, rtype_index_visit<Fn&&, V&&>>(SWL_FWD(fn),
                                                                             SWL_FWD(v));
}

template <class Fn, class Head, class... Tail>
constexpr decltype(auto) multi_visit(Fn&& fn, Head&& head, Tail&&... tail) {
    // visit them one by one, starting with the last
    auto vis = [&fn, &head](auto&&... args) -> decltype(auto) {
        return vimpl::visit(
            [&fn, &args...](auto&& elem) -> decltype(auto) {
                return SWL_FWD(fn)(SWL_FWD(elem), SWL_FWD(args)...);
            },
            SWL_FWD(head));
    };

    if constexpr (sizeof...(tail) == 0)
        return SWL_FWD(vis)();
    else if constexpr (sizeof...(tail) == 1)
        return vimpl::visit(SWL_FWD(vis), SWL_FWD(tail)...);
    else
        return vimpl::multi_visit(SWL_FWD(vis), SWL_FWD(tail)...);
}

#undef DEC
#undef SEQ30
#undef SEQ100
#undef SEQ200
#undef SEQ400
#undef DeclareUnreachable
#undef CAT
#undef CAT2
#undef INJECTSEQ

}  // namespace v1

#endif