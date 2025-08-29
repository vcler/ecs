#pragma once

#include <atomic>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace ecs {
namespace detail {

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
constexpr bool pairwise_distinct = (is_unique<Ts, Ts...>() && ...);

template <class T>
auto type_hash()
{
    return typeid(T).hash_code();
}

template <class... Ts>
auto xor_type_hash()
{
    // disallow same types to keep randomness
    static_assert(pairwise_distinct<Ts...>);

    return (type_hash<Ts> ^ ...);
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

} // namespace detail
} // namespace ecs

