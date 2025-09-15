# ecs

DISCLAIMER: Don't use this library, use as a reference for your own implementation.

Itsy-bitsy header only C++23 entity component system inspired by the great talk [Overwatch Gameplay Architecture and Netcode](https://www.youtube.com/watch?v=W3aieHjyNvw) from Tim Ford.
Also includes a (poor) implementation of the C++26 [std::hive proposal](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0447r22.html), probably the best container for storing components.
For a complete overview of supported features, see the documented interface in ecs.hpp.

Entities are just handles associated with a set of components.
You cannot (and should not) iterate over entities.
You can, however, iterate over tuples of components, that is, all entities associated with the tuple.
Components are stored in (mostly) contigous memory.
There is some overhead whenever a new range (for iteration of tuples) is first created, but no further cost for later uses.
Also when using ranges, the order of components in tuples does not matter, the same internal range is reused.
Abuses type erasure to reduce the number of templates forced onto the user. As of right now, error handling is exception based - no expected values YET.

## Example usage

```c++
#include <ecs.hpp>

struct pos {
    float x;
    float y;
};

struct vel {
    float dx;
    float dy;
};

struct hp {
    float value;
    float max;
    // will be set by registry
    ecs::handle_type owner{};
};

void damage_system(ecs::registry &reg)
{
    for (auto &[pos, hp] : ecs::range<pos, hp>(reg)) {
        // deal damage
        if (pos.x == pos.y) {
            hp.value = std::max(0.f, hp.value - 1.f);

            // knockback dynamic entities
            if (ecs::has_sibling<vel>(reg, hp))
                ecs::sibling<vel>(reg, hp).dx += 1.f;
        }
    }

    // regen
    for (auto &hp : ecs::range<hp>(reg)) {
        hp.value = std::min(hp.max, hp.value + .1f);
    }
}

int main()
{
    ecs::registry reg;

    // create an entity with position and hitpoints
    ecs::create(reg, pos(1.f, 1.f), hp(64.f, 64.f));

    auto ent = ecs::create(reg, pos(0.f, 0.f), vel(0.f, 0.f));
    // add components after creation
    ecs::emplace(reg, ent, hp(32.f, 64.f));

    damage_system(reg);

    // components must be known for destruction
    ecs::destroy<pos, vel, hp>(reg, ent);

    // ~ecs::registry() destroys remains
    return 0;
}

```

## Dependencies
* g++ >= 14.2.0
* boost::dynamic_bitset
* doctest (testing only)
* cmake (testing only)

## Build
To build the tests, copy a doctest header into the test/ directory
Then, run
```bash

cmake -B build && cd build && make -j

```

