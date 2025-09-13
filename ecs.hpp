#pragma once

#include <registry.hpp>

namespace ecs {

template <class... Cs>
handle_type create(registry &reg, Cs &&...args)
{
    return reg.create(std::forward<Cs>(args)...);
}

template <class... Cs>
void destroy(registry &reg, handle_type ent)
{
    reg.destroy<Cs...>(ent);
}

template <class C>
C &get(registry &reg, handle_type ent)
{
    reg.get<C>(ent);
}

template <class C, class... Cs>
auto range(registry &reg)
{
    return reg.range<C, Cs...>();
}

template <class C>
C &emplace(registry &reg, handle_type ent, C &&arg)
{
    return reg.emplace(ent, std::forward<C>(arg));
}

template <class C, class... Args>
C &emplace(registry &reg, handle_type ent, Args &&...args)
{
    return reg.emplace<C>(ent, std::forward<Args>(args)...);
}

template <class S>
S &singleton(registry &reg)
{
    return reg.singleton<S>();
}

template <class S>
S &singleton(registry &reg, S &&singleton)
{
    return reg.singleton(std::forward<S>(singleton));
}

template <detail::FatComponent C>
handle_type entity_of(registry &reg, const C &comp) noexcept
{
    return reg.entity_of(comp);
}

template <class C, detail::FatComponent F>
bool has_sibling(registry &reg, const F &comp)
{
    return reg.has_sibling<C>(comp);
}

template <class C, detail::FatComponent F>
C &sibling(registry &reg, const F &comp)
{
    return reg.sibling<C>(comp);
}

} // namespace ecs

