#pragma once

#include <unordered_set>

namespace ecs {

struct component {
    // TODO: perf: replace pos with pointer
    size_t hash;
    size_t pos;

    struct hash_fn {
        size_t operator()(const component &arg) const noexcept
        {
            return arg.hash;
        }
    };

    struct comparison_fn {
        bool operator()(const component &lhs,
            const component &rhs) const noexcept
        {
            return lhs.hash == rhs.hash;
        }
    };
};

using component_set = std::unordered_set<component,
        component::hash_fn, component::comparison_fn>;

} // namespace ecs

