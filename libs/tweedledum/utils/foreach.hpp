/*--------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
*-------------------------------------------------------------------------------------------------*/
#pragma once

#include <cstdint>
#include <type_traits>

/* Taken from mockturtle (: */

namespace tweedledum {

template <class Fn, class ElementType, class ReturnType>
constexpr auto is_callable_with_index_v =
    std::is_invocable_r_v<ReturnType, Fn, ElementType&, uint32_t>;

template <class Fn, class ElementType, class ReturnType>
constexpr auto is_callable_without_index_v =
    std::is_invocable_r_v<ReturnType, Fn, ElementType&>;

/* \brief Applies a function to a range of elements
 *
 * The parameter ``fn`` is any callable that must have one of the following four
 * signatures.
 * - ``void(element)``
 * - ``void(element, element_index)``
 * - ``bool(element)``
 * - ``bool(element, element_index)``
 */
template <class Iterator, class ElementType = typename Iterator::value_type,
          class Fn>
Iterator foreach_element(Iterator begin, Iterator end, Fn&& fn,
                         uint32_t counter_offset = 0) {
    // clang-format off
	static_assert(is_callable_with_index_v<Fn, ElementType&, void> ||
	              is_callable_without_index_v<Fn, ElementType&, void> ||
		      is_callable_with_index_v<Fn, ElementType&, bool> ||
		      is_callable_without_index_v<Fn, ElementType&, bool>);
    // clang-format on

    if constexpr (is_callable_without_index_v<Fn, ElementType&, bool>) {
        (void) counter_offset;
        while (begin != end) {
            if (!fn(*begin++)) {
                return begin;
            }
        }
        return begin;
    } else if constexpr (is_callable_with_index_v<Fn, ElementType&, bool>) {
        uint32_t index{counter_offset};
        while (begin != end) {
            if (!fn(*begin++, index++)) {
                return begin;
            }
        }
        return begin;
    } else if constexpr (is_callable_without_index_v<Fn, ElementType&, void>) {
        (void) counter_offset;
        while (begin != end) {
            fn(*begin++);
        }
        return begin;
    } else if constexpr (is_callable_with_index_v<Fn, ElementType&, void>) {
        uint32_t index{counter_offset};
        while (begin != end) {
            fn(*begin++, index++);
        }
        return begin;
    }
}

/* \brief Applies a function to elements satisfying specific criteria in a range
 * of elements
 *
 * The parameter ``predicate`` is any callable that have the following
 * signature.
 * - ``bool(element)``
 *
 * The parameter ``fn`` is any callable that must have one of the following four
 * signatures.
 * - ``void(element)``
 * - ``void(element, element_index)``
 * - ``bool(element)``
 * - ``bool(element, element_index)``
 */
template <class Iterator, class ElementType = typename Iterator::value_type,
          class Pred, class Fn>
Iterator foreach_element_if(Iterator begin, Iterator end, Pred&& predicate,
                            Fn&& fn, uint32_t counter_offset = 0) {
    // clang-format off
	static_assert(is_callable_with_index_v<Fn, ElementType&, void> ||
	              is_callable_without_index_v<Fn, ElementType&, void> ||
		      is_callable_with_index_v<Fn, ElementType&, bool> ||
		      is_callable_without_index_v<Fn, ElementType&, bool>);
	static_assert(is_callable_without_index_v<Pred, ElementType&, bool>);
    // clang-format on

    if constexpr (is_callable_without_index_v<Fn, ElementType, bool>) {
        (void) counter_offset;
        while (begin != end) {
            if (!predicate(*begin)) {
                ++begin;
                continue;
            }
            if (!fn(*begin++)) {
                return begin;
            }
        }
        return begin;
    } else if constexpr (is_callable_with_index_v<Fn, ElementType, bool>) {
        uint32_t index = counter_offset;
        while (begin != end) {
            if (!predicate(*begin)) {
                ++begin;
                ++index;
                continue;
            }
            if (!fn(*begin++, index++)) {
                return begin;
            }
        }
        return begin;
    } else if constexpr (is_callable_without_index_v<Fn, ElementType, void>) {
        (void) counter_offset;
        while (begin != end) {
            if (!predicate(*begin)) {
                ++begin;
                continue;
            }
            fn(*begin++);
        }
        return begin;
    } else if constexpr (is_callable_with_index_v<Fn, ElementType, void>) {
        uint32_t index = counter_offset;
        while (begin != end) {
            if (!predicate(*begin)) {
                ++begin;
                ++index;
                continue;
            }
            fn(*begin++, index++);
        }
        return begin;
    }
}

} // namespace tweedledum
