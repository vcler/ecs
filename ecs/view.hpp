// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <span>
#include <type_traits>
#include <unordered_set>

#include <ecs/detail/types.hpp>
#include <ecs/detail/colony.hpp>
#include <ecs/component.hpp>

namespace ecs {

template <class... Cs>
class view {
    using component_types = std::tuple<Cs...>;

public:
    view(size_t *order, void **components);

    template <size_t I>
    auto &get();

private:
    size_t *order_;
    void **components_;
};

} // namespace ecs

namespace std {
// structured binding support

template <class... Cs>
struct tuple_size<ecs::view<Cs...>> {
    static constexpr size_t value = sizeof...(Cs);
};

template <size_t I, class... Cs>
struct tuple_element<I, ecs::view<Cs...>> {
    using type = std::tuple_element_t<I, std::tuple<Cs...>>;
};

} // namespace std

namespace ecs {

struct view_range {
    std::unordered_set<size_t> types;
    std::vector<void *> views;

    size_t size() const noexcept;
    void push_back(size_t entity, std::span<void *> ptrs);
    void erase(size_t entity);
    bool captures(const component_set &comps) const noexcept;
};

namespace views {

template <class... Cs>
class iterator;

class sentinel {
public:
    sentinel(void **pos) : pos_(pos) { }
    void **pos() const noexcept { return pos_; }

private:
    void **pos_;
};

template <class... Cs>
class iterator {
    static constexpr auto stride = sizeof...(Cs);
public:
    iterator(void **pos,
        const std::unordered_set<size_t> &types);

    bool operator==(const iterator &rhs) const noexcept;
    bool operator==(const sentinel &sentinel) const noexcept;
    view<Cs...> &operator*();
    iterator operator++();
    iterator operator++(int);

private:
    void **pos_;

    // must own the view so we can return a view &
    // from operator *. This allows the caller to
    // bind to component &, i.e. auto &[x, y] = *it;
    // which is not possible if we return an rvalue
    view<Cs...> view_;

    //  ranges can be created with different order,
    //  i.e range<X, Y> or range<Y, X>. Instead of
    //  constructing a new range for every combination,
    //  reuse the original range and map the indices
    std::array<size_t, stride> order_;
};

} // namespace views

template <class... Cs>
class typed_view_range {
    // purpose: restore information lost by view_range
    // due to type erasure
public:
    typed_view_range(view_range &range);

    views::iterator<Cs...> begin() noexcept;
    views::sentinel end() noexcept;

private:
    view_range &range_;
};

inline size_t view_range::size() const noexcept
{
    return types.size();
}

inline void view_range::push_back(
    size_t entity, std::span<void *> ptrs)
{
    views.reserve(views.size() + ptrs.size() + 1);
    views.push_back(reinterpret_cast<void *>(entity));
    views.insert(views.end(), ptrs.rbegin(), ptrs.rend());
}

inline void view_range::erase(size_t entity)
{
    const int stride = types.size() + 1;

    for (auto it = views.begin(); it != views.end();
        it += stride)
    {
        if (reinterpret_cast<size_t>(*it) != entity)
            continue;

        if (std::distance(it, views.end()) < stride)
            throw std::out_of_range("view corruption");

        views.erase(it, it + stride);
        return;
    }

    throw std::out_of_range("entity not found");
}

inline bool view_range::captures(
    const component_set &comps) const noexcept
{
    if (comps.size() < types.size())
        return false;

    for (const auto hash : types) {
        if (!comps.contains({ hash, 0 }))
            return false;
    }

    return true;
}



template <class... Cs>
view<Cs...>::view(size_t *order, void **components)
    : order_(order)
    , components_(components)
{
}

template <class... Cs>
template <size_t I>
auto &view<Cs...>::get()
{
    using type = std::tuple_element<
            I, component_types>::type;

    static_assert(I < sizeof...(Cs));

    return *static_cast<type *>(components_[order_[I]]);
}

template <class... Cs>
typed_view_range<Cs...>::typed_view_range(view_range &range)
    : range_(range)
{
}

template <class... Cs>
views::iterator<Cs...> typed_view_range<Cs...>::begin() noexcept
{
    return views::iterator<Cs...>(
            range_.views.data(), range_.types);
}

template <class... Cs>
views::sentinel typed_view_range<Cs...>::end() noexcept
{
    return views::sentinel(
            range_.views.data() + range_.views.size());
}

namespace views {

template <class... Cs>
iterator<Cs...>::iterator(void **pos,
    const std::unordered_set<size_t> &types)
    : pos_(pos)
    , view_(nullptr, nullptr)
{
    auto const type_index = [&types](auto t)
    {
        using type = typename decltype(t)::type;
        const auto hash = detail::type_hash<type>();

        return std::distance(types.find(hash),
                types.end()) - 1;
    };

    auto it = order_.begin();
    (..., (*it++ = type_index(
            std::type_identity<Cs>{})));
}

template <class... Cs>
bool iterator<Cs...>::operator==(
    const iterator &rhs) const noexcept
{
    return pos_ == rhs.pos_;
}

template <class... Cs>
bool iterator<Cs...>::operator==(
    const sentinel &sentinel) const noexcept
{
    return pos_ == sentinel.pos();
}

template <class... Cs>
view<Cs...> &iterator<Cs...>::operator*()
{
    view_ = view<Cs...>(order_.data(), pos_ + 1);
    return view_;
}

template <class... Cs>
iterator<Cs...> iterator<Cs...>::operator++()
{
    pos_ += stride + 1;
    return *this;
}

template <class... Cs>
iterator<Cs...> iterator<Cs...>::operator++(int)
{
    iterator temp(*this);
    ++*this;
    return temp;
}

} // namespace views
} // namespace ecs

