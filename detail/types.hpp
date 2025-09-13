// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <detail/colony.hpp>

namespace ecs {

using handle_type = size_t;

namespace detail {

template <class C>
using storage_type = colony<std::remove_cvref_t<C>>;

template <class T, class... Us>
constexpr bool is_one_of = (std::is_same_v<T, Us> || ...);

template <class Ts, class Us>
constexpr bool is_subset_of = false;

template <class... Ts, class... Us>
constexpr bool is_subset_of<
        std::tuple<Ts...>, std::tuple<Us...>>
    = (is_one_of<Ts, Us...> && ...);

template <class T, class... Us>
consteval bool is_unique()
{
    auto count = (std::is_same_v<T, Us> + ...);
    return count == 1;
}

template <class... Ts>
using head_type = std::tuple_element_t<0uz, std::tuple<Ts...>>;

template <class T, class... Us>
consteval size_t type_index()
{
    static_assert(is_unique<T, Us...>());

    size_t index = 0;
    return ((std::is_same_v<T, Us> * index++) + ...);
}

template <class... Ts>
constexpr bool pairwise_distinct =
        (is_unique<Ts, Ts...>() && ...);

template <class T>
auto type_hash()
{
    // typeid discards cvref qualifiers
    return typeid(T).hash_code();
}

template <class... Ts>
auto xor_type_hash()
{
    // disallow same types to keep randomness
    static_assert(pairwise_distinct<Ts...>);

    return (type_hash<Ts>() ^ ...);
}

template <class T, size_t>
using remove_size_t = T;

template <class T, size_t... Is>
consteval auto repeated_tuple_impl(std::index_sequence<Is...>)
{
    return std::tuple<head_type<T, decltype(Is)>...>{};
}

template <class T, size_t N>
using repeated_tuple = decltype(
        repeated_tuple_impl<T>(std::make_index_sequence<N>()));

template <class C>
concept FatComponent = requires(const C &comp)
{
    // stores handle of the entity it is owned by
    requires std::same_as<decltype(comp.owner), handle_type>;
};

} // namespace detail
} // namespace ecs

