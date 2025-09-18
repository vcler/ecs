// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <ecs/component.hpp>
#include <ecs/detail/colony.hpp>
#include <ecs/detail/types.hpp>
#include <ecs/view.hpp>

namespace ecs {

class registry {
    using size_type = size_t;

public:
    registry() = default;
    registry(const registry &) = delete;
    registry(registry &&) = default;
    ~registry() = default;

    template <class... Cs>
    handle_type create(Cs &&...args);
    handle_type create();

    void destroy(handle_type ent);

    template <class C>
    C &get(handle_type ent);

    bool contains(handle_type ent) const noexcept;

    template <class C, class... Cs>
    auto range();

    template <class C>
    C &emplace(handle_type ent, C &&arg);
    template <class C, class... Args>
    C &emplace(handle_type ent, Args &&...args);

    template <class S>
    S &singleton();
    template <class S>
    S &singleton(S &&singleton);

    template <detail::FatComponent C>
    handle_type entity_of(const C &comp) const noexcept;

    template <class C, detail::FatComponent F>
    bool has_sibling(const F &comp) const;
    template <class C, detail::FatComponent F>
    C &sibling(const F &comp);

private:
    template <class C>
    detail::storage_type<C> &storage_for();

    template <class... Cs>
    typed_view_range<Cs...> range_for();

    template <class C>
    std::tuple<size_type, C *>
    construct_component(handle_type owner, C &&comp);

    template <class C>
    void destroy_component(handle_type ent);

    using entity_dtor_fn = std::function<
            void(registry &, handle_type)>;

    entity_dtor_fn placeholder_dtor();
    template <class... Cs>
    entity_dtor_fn entity_dtor();
    template <class... Cs>
    entity_dtor_fn nested_entity_dtor(entity_dtor_fn &&dtor);

    struct entinfo {
        entinfo(size_t hash, component_set &&comps,
            entity_dtor_fn dtor)
            : xor_hash(hash)
            , components(std::move(comps))
            , dtor(dtor)
        { }

        size_t xor_hash;
        component_set components;
        entity_dtor_fn dtor;
    };

    handle_type max_entity_handle_ = 1uz;
    std::unordered_map<size_type,
            std::shared_ptr<void>> components_;
    std::unordered_map<handle_type, entinfo> entities_;
    std::unordered_map<size_type, view_range> ranges_;
    std::unordered_map<size_type,
            std::shared_ptr<void>> singletons_;
};

template <class C>
class component_range {
public:
    using iterator = detail::storage_type<C>::iterator;
    using const_iterator = const iterator;

    component_range(detail::storage_type<C> &components);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    detail::storage_type<C> &components_;
};

} // namespace ecs

namespace ecs {

template <class... Cs>
handle_type registry::create(Cs &&...args)
{
    static_assert(detail::pairwise_distinct<Cs...>);

    const handle_type ent = max_entity_handle_++;
    auto comps = component_set{};
    comps.reserve(sizeof...(Cs));
    // ptrs to components for constucting views
    std::unordered_map<size_type, void *> ptrs;
    ptrs.reserve(sizeof...(Cs));

    const auto ctor = [&](auto &&arg)
    {
        using type = std::remove_cvref_t<decltype(arg)>;
        const auto hash = detail::type_hash<type>();

        auto [pos, ptr] = construct_component(ent,
                std::forward<decltype(arg)>(arg));

        comps.emplace(hash, ptr);
        ptrs.emplace(hash, static_cast<void *>(ptr));
    };

    (..., ctor(std::forward<Cs>(args)));

    // update views
    std::array<void *, sizeof...(Cs)> new_view;

    for (auto &[xor_hash, range] : ranges_) {
        if (!range.captures(comps)) {
            continue;
        }

        // add entity to the range
        auto it = std::begin(new_view);
        
        for (const size_t hash : range.types) {
            *it++ = ptrs.at(hash);
        }

        range.push_back(ent, std::span(
                new_view.data(), std::size(range)));
    }

    const auto xor_hash = detail::xor_type_hash<Cs...>();
    entities_.emplace(ent,
            entinfo(xor_hash, std::move(comps),
                entity_dtor<Cs...>()));

    return ent;
}

inline handle_type registry::create()
{
    const handle_type ent = max_entity_handle_++;
    const auto xor_hash = 0uz;

    entities_.emplace(ent, entinfo(xor_hash, component_set{},
            placeholder_dtor()));

    return ent;
}

template <class C>
detail::storage_type<C> &registry::storage_for()
{
    const auto hash = detail::type_hash<C>();

    if (!components_.contains(hash)) {
        components_.emplace(hash,
                std::make_unique<detail::storage_type<C>>());
    }

    return *reinterpret_cast<detail::storage_type<C> *>(
            components_.at(hash).get());
}

template <class C, class... Cs>
auto registry::range()
{
    if constexpr (sizeof...(Cs) == 0) {
        return component_range<C>(storage_for<C>());
    } else {
        return range_for<C, Cs...>();
    }
}

template <class... Cs>
typed_view_range<Cs...> registry::range_for()
{
    static_assert(detail::pairwise_distinct<Cs...>);

    const auto xor_hash = detail::xor_type_hash<Cs...>();

    if (ranges_.contains(xor_hash)) {
        return typed_view_range<Cs...>(ranges_.at(xor_hash));
    }

    // construct the range
    view_range range;
    range.types.reserve(sizeof...(Cs));
    (range.types.emplace(detail::type_hash<Cs>()), ...);

    // NOTE: perf: instead of checking each
    // individual entities' component hashes for a
    // collision with the view's types, remember which
    // component xor hashes fit and which don't so that
    // the collision only has to be computed once for
    // every unique set of components (entity type)
    // TODO: perf: probably faster just using a vector
    std::unordered_set<size_type> included;
    std::unordered_set<size_type> excluded;

    std::array<void *, sizeof...(Cs)> view;

    for (const auto &[ent, info] : entities_) {
        if (excluded.contains(info.xor_hash)) {
            continue;
        }

        if (!included.contains(info.xor_hash)) {
            if (!range.captures(info.components)) {
                excluded.emplace(info.xor_hash);
                continue;
            }
            included.emplace(info.xor_hash);
        }

        // create a new view
        auto it = view.begin();
        for (const auto hash : range.types) {
            *it++ = info.components.find({ hash, 0 })->ptr;
        }

        range.push_back(ent, std::span(
                view.data(), std::size(range)));
    };

    ranges_.emplace(xor_hash, std::move(range));
    return typed_view_range<Cs...>(ranges_.at(xor_hash));
}

template <class C>
std::tuple<registry::size_type, C *>
registry::construct_component(handle_type owner, C &&arg)
{
    auto &stor = storage_for<C>();
    size_type pos = stor.push_back(std::forward<C>(arg));

    auto &comp = stor.at(pos);
    if constexpr (detail::FatComponent<C>) {
        // set owner
        comp.owner = owner;
    } else {
    }

    return std::make_tuple(pos, &(stor.at(pos)));
}

template <class C>
C &registry::get(handle_type ent)
{
    if (!entities_.contains(ent))
        throw std::out_of_range("no such entity");

    const auto &comps = entities_.at(ent).components;

    auto it = comps.find({ detail::type_hash<C>(), 0 });
    if (it == std::end(comps))
        throw std::invalid_argument("no such component");

    return *static_cast<C *>(it->ptr);
}

inline bool registry::contains(handle_type ent) const noexcept
{
    return entities_.contains(ent);
}

inline void registry::destroy(handle_type ent)
{
    if (!entities_.contains(ent))
        throw std::out_of_range("no such entity");

    // remove views
    const auto &info = entities_.at(ent);
    for (auto &[xor_hash, range] : ranges_) {
        if (range.captures(info.components))
            range.erase(ent);
    }

    // destoy components
    info.dtor(*this, ent);

    entities_.erase(ent);
}

template <class C>
C &registry::emplace(handle_type ent, C &&arg)
{
    if (!entities_.contains(ent))
        throw std::out_of_range("no such entity");

    const auto hash = detail::type_hash<C>();
    auto &info = entities_.at(ent);
    auto &comps = info.components;
    if (comps.contains({ hash, 0 }))
        throw std::logic_error("duplicate component");

    auto [pos, ptr] = construct_component(ent,
            std::forward<C>(arg));

    comps.emplace(hash, ptr);
    info.xor_hash ^= hash;
    info.dtor = nested_entity_dtor<C>(std::move(info.dtor));

    // update view
    std::vector<void *> view(comps.size() + 1uz);

    for (auto &[xor_hash, range] : ranges_) {
        if (!range.types.contains(hash))
            continue;

        // candidate
        if (!range.captures(comps))
            continue;

        // create a new view
        auto it = view.data();
        for (const auto hash : range.types) {
            *it++ = comps.find({ hash, 0 })->ptr;
        }

        range.push_back(ent, std::span(
            view.data(), view.capacity()));
    }

    return *ptr;
}

template <class C, class... Args>
C &registry::emplace(handle_type ent, Args &&...args)
{
    // extra move but less code
    return emplace(ent, C(std::forward<Args>(args)...));
}

inline registry::entity_dtor_fn registry::placeholder_dtor()
{
    // for componentless entities
    return [](registry &, handle_type) { };
}

template <class... Cs>
registry::entity_dtor_fn registry::entity_dtor()
{
    // destroys all components Cs
    return [](registry &reg, handle_type ent)
    {
        (..., reg.destroy_component<Cs>(ent));
    };
}

template <class... Cs>
registry::entity_dtor_fn registry::nested_entity_dtor(
    entity_dtor_fn &&previous_dtor)
{
    // if components are added after creation with
    // emplace(), they cant't be destoyed by the
    // original dtor.  So we create a new dtor to
    // destroy the emplace'd component, and then
    // call the previous dtor to destroy the rest.
    return [previous_dtor](registry &reg, handle_type ent)
    {
        previous_dtor(reg, ent);
        (..., reg.destroy_component<Cs>(ent));
    };
}

template <class C>
void registry::destroy_component(handle_type ent)
{
    const auto &comps = entities_.at(ent).components;

    auto it = comps.find({ detail::type_hash<C>(), 0 });
    if (it == std::end(comps))
        throw std::invalid_argument("no such component");

    storage_for<C>().erase(static_cast<C *>(it->ptr));
}

template <class S>
S &registry::singleton()
{
    const auto hash = detail::type_hash<S>();
    if (!singletons_.contains(hash))
        throw std::out_of_range("no such singleton");

    return *static_cast<S *>(singletons_.at(hash).get());
}

template <class S>
S &registry::singleton(S &&arg)
{
    const auto hash = detail::type_hash<S>();

    if (singletons_.contains(hash)) {
        // throw, duplicate emplace has no effect
        throw std::logic_error("singleton exists");
    }

    singletons_.emplace(hash,
            std::make_unique<S>(std::forward<S>(arg)));

    return *static_cast<S *>(singletons_.at(hash).get());
}

template <detail::FatComponent C>
handle_type registry::entity_of(const C &comp) const noexcept
{
    return comp.owner;
}

template <class C, detail::FatComponent F>
bool registry::has_sibling(const F &comp) const
{
    const auto ent = entity_of(comp);
    if (!entities_.contains(ent))
        throw std::out_of_range("no such entity");

    const auto &[xor_hash, comps, dtor] = entities_.at(ent);
    return comps.contains({ detail::type_hash<C>(), 0 });
}

template <class C, detail::FatComponent F>
C &registry::sibling(const F &comp)
{
    return get<C>(entity_of(comp));
}

template <class C>
component_range<C>::component_range(
    detail::storage_type<C> &components)
    : components_(components)
{
}

template <class C>
component_range<C>::iterator component_range<C>::begin()
{
    return components_.begin();
}

template <class C>
component_range<C>::iterator component_range<C>::end()
{
    return components_.end();
}

template <class C>
component_range<C>::const_iterator
component_range<C>::begin() const
{
    return components_.begin();
}

template <class C>
component_range<C>::const_iterator
component_range<C>::end() const
{
    return components_.end();
}

} // namespace ecs


