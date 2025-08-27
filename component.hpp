#pragma once

#include <unordered_set>

namespace ecs {

struct component {
    size_t hash;
    size_t pos;

    bool operator==(const component &rhs) const { return hash == rhs.hash; }
    bool operator==(size_t rhs) const { return hash == rhs; };
};

struct component_hash_fn {
    size_t operator()(const component &arg) const { return arg.hash; }
};

using component_set = std::unordered_set<component, component_hash_fn>;

} // namespace ecs

