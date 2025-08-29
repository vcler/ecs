#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <colony.hpp>
#include <component.hpp>
#include <detail.hpp>

namespace ecs {

class registry {
    using size_type = size_t;
    using handle_type = size_t;

    template <class C>
    struct component_wrapper;
public:
    registry() = default;
    registry(const registry &) = delete;
    registry(registry &&) = default;
    ~registry() = default;

    template <class... Cs>
    handle_type allocate(Cs &&...args);

    template <class... Cs>
    void destroy(handle_type ent);

    template <class C>
    C &get(handle_type ent);

private:
    template <class C>
    struct component_wrapper {
        handle_type entity;
        C component;
    };

    template <class C>
    using storage = colony<component_wrapper<C>>;

    template <class C>
    storage<C> &storage_for();

    template <class C>
    storage<C>::size_type construct_component(
        handle_type owner, C &&comp);

    template <class C>
    void destroy_component(handle_type owner);

    handle_type entity_counter_ = 0uz;
    std::unordered_map<size_type, std::shared_ptr<void>> components_;
    std::unordered_map<handle_type, component_set> entities_;
};

template <class... Cs>
registry::handle_type registry::allocate(Cs &&...args)
{
    const handle_type ent = entity_counter_++;
    auto comps = component_set{};
    comps.reserve(sizeof...(args));

    (comps.emplace(detail::type_hash<Cs>(),
            construct_component<Cs>(
                ent, std::forward<Cs>(args))), ...);

    entities_.emplace(ent, std::move(comps));

    return ent;
}

template <class C>
registry::storage<C> &registry::storage_for()
{
    const auto hash = detail::type_hash<C>();

    if (!components_.contains(hash)) {
        components_.emplace(hash,
                std::make_shared<storage<C>>());
    }

    return *reinterpret_cast<storage<C> *>(
            components_.at(hash).get());
}

template <class C>
registry::storage<C>::size_type
registry::construct_component(
    handle_type owner, C &&comp)
{
    return storage_for<C>()
            .emplace_back(owner, std::forward<C>(comp));
}

template <class C>
C &registry::get(handle_type ent)
{
    if (!entities_.contains(ent))
        throw std::out_of_range("no such entity");

    const auto &comps = entities_.at(ent);

    auto it = comps.find({ detail::type_hash<C>(), 0 });
    if (it == std::end(comps))
        throw std::out_of_range("no such component");

    return storage_for<C>().at(it->pos).component;
}

template <class... Cs>
void registry::destroy(handle_type ent)
{
    if (!entities_.contains(ent))
        throw std::out_of_range("no such entity");

    (destroy_component<Cs>(ent), ...);
    entities_.erase(ent);
}

template <class C>
void registry::destroy_component(handle_type owner)
{
    const auto &comps = entities_.at(owner);

    auto it = comps.find({ detail::type_hash<C>(), 0 });
    if (it == std::end(comps))
        throw std::out_of_range("no such component");

    storage_for<C>().erase(it->pos);
}

} // namespace ecs


