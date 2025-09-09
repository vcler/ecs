#pragma once

#include <cstddef>
#include <colony.hpp>

namespace ecs {

/*
template <class C>
struct component_wrapper {
    size_t entity;
    C component;
};
*/

template <class C>
using storage_type = colony<C>;

} // namespace ecs

