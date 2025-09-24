// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ecs/registry.hpp>

namespace ecs {

/** Returns a handle to a newly created entity.

    The components of an entity must pairwise distinct, i.e.
    an entity can only have a single compnent of every type.

    @note Further components can be added to the entity after
    creation using the emplace() function.
    
    @tparam Cs Components the entity will be associated with. 

    @param reg

    @param components Arguments forwarded to initialize the
    components.
*/
template <class... Cs>
handle_type create(registry &reg, Cs &&...components)
{
    return reg.create(std::forward<Cs>(components)...);
}

/** Returns a handle to a newly created entity.

    This entity is associated with no components, but new
    components can be added with the emplace() function.

    @param reg
*/
inline handle_type create(registry &reg)
{
    return reg.create();
}

/** Destroy an existing entity and it's components.

    @throws out_of_range if the entity does not exist or
    invalid_argument if the entity is not associated with
    the specified component.

    @param reg

    @param ent The handle of the entity to be destroyed.
*/
inline void destroy(registry &reg, handle_type ent)
{
    reg.destroy(ent);
}

/** Destroy an existing entity and it's components.

    @throws out_of_range if the entity does not exist or
    invalid_argument if the entity is not associated with
    the specified component.

    @param reg

    @param comp Any component associated with the entity
    that will be destroyed.

    @tparam F Component type, must be a fat component.
*/
template <detail::FatComponent F>
inline void destroy(registry &reg, F const &comp)
{
    reg.destroy(reg.entity_of(comp));
}

/** Returns the component of an entity.

    @throws out_of_range if the entity does not exist or
    invalid_argument if the entity is not associated with
    the specified component.

    @param reg

    @param ent The entity that owns the component.

    @tparam The type of the component to get.

*/
template <class C>
C &get(registry &reg, handle_type ent)
{
    return reg.get<C>(ent);
}

/** Check if the registry owns an entity.
    
    @param reg

    @param ent Entity to check for.
*/
inline bool contains(registry &reg, handle_type ent) noexcept
{
    return reg.contains(ent);
}

/** Returns a range to iterate over all components of a type
    or component tuples.

    Component Tuples allow you to iterate over all entities
    that are associated with the specified tuple, regardless
    of what other components they may own.

    @param reg

    @tparam C The component type to iterate over.

    @tparam Cs Optional, further component types that will
    allow you to iterate over component tuples.

*/
template <class C, class... Cs>
auto range(registry &reg)
{
    return reg.range<C, Cs...>();
}

/** Returns a reference to the component that is added to
    the entity.

    @throws out_of_range if the entity does not exist or
    logic_error if the entity is already associated with
    a component of this type.

    @param reg

    @param component The argument that is forwarded to
    construct the component.

    @tparam C The type of the component.

*/
template <class C>
C &emplace(registry &reg, handle_type ent, C &&component)
{
    return reg.emplace(ent, std::forward<C>(component));
}

/** Similar to the other emplace, but constructs the
    component in place.

    @tparam Args Arguments used to construct the component.
*/
template <class C, class... Args>
C &emplace(registry &reg, handle_type ent, Args &&...args)
{
    return reg.emplace<C>(ent, std::forward<Args>(args)...);
}

/** Returns a reference to a singleton.
    
    Every registry can only store a single instance of every
    Singleton type.  Singletons are also not associated with
    entities.

    @throws out_of_range if the singleton does not exist.
    Create it first using the other singleton() overload.

    @param reg

    @tparam S Type of the singleton to get.
*/
template <class S>
S &singleton(registry &reg)
{
    return reg.singleton<S>();
}

/** Returns a reference to a newly created singleton.

    @throws logic_error if the singleton already exists.

    @param reg

    @param singleton Argument forwarded to construct the
    singleton from.
    
    @tparam S Type of the singleton.
*/
template <class S>
S &singleton(registry &reg, S &&singleton)
{
    return reg.singleton(std::in_place_t{},
            std::forward<S>(singleton));
}

/** Returns a reference to a newly created singleton.
    The singleton is created in place.

    @throws logic_error if the singleton already exists.

    @tparam S Type of the singleton.

    @tparam Args

    @param reg

    @param args Arguments forwarded to construct the
    singleton from.
*/
template <class S, class... Args>
S &singleton(registry &reg, std::in_place_t, Args &&...args)
{
    return reg.singleton<S>(std::forward<Args>(args)...);
}

/** Returns the handle of the entity that owns the component.
    
    @note The component must satisfy the FatComponent
    constraint, that is, the component must have a member
    called 'owner' which stores the entity handle.
    Any FatComponent that is associated with an entity will
    automatically have it set by the registry.

    @param component A component of the returned entity.

    @tparam F Type of the entity.
*/
template <detail::FatComponent F>
handle_type entity_of(const F &component) noexcept
{
    return component.owner;
}

/** Returns true if the entity that owns component also owns
    a component of type C.

    @throws out_of_range if the entity does not exist.

    @param reg

    @param component The component that is queried for.

    @tparam C Type of the sibling candiate.

    @tparam F Type of component that is queried for.
*/
template <class C, detail::FatComponent F>
bool has_sibling(registry &reg, const F &component)
{
    return reg.has_sibling<C>(component);
}

/** Returns a reference to the sibling component.

    @throws out_of_range if the entity does not exist
    or invalid_argument if the entity is not associated
    with the sibling component.

    @param reg

    @param component The component that is queried for.

    @tparam C Type of the sibling candiate.

    @tparam F Type of component that is queried for.

*/
template <class C, detail::FatComponent F>
C &sibling(registry &reg, const F &component)
{
    return reg.sibling<C>(component);
}

} // namespace ecs

