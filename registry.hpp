#pragma once

#include <memory>
#include <unordered_map>

#include <component.hpp>
#include <vector>


namespace ecs {

class registry {
    using size_type = size_t;
    using handle_type = size_t;
public:
    registry() = default;
    registry(const registry &) = delete;
    registry(registry &&) = default;
    ~registry() = default;

    template <class... Cs>
    handle_type emplace(Cs &&...args);

    template <class... Cs>
    void destroy(handle_type ent);

    template <class C>
    C &get(handle_type ent);

private:
    std::unordered_map<size_type, std::shared_ptr<void>> components_;
    std::unordered_map<handle_type, std::vector<component_set>> entities_;
};

} // namespace ecs


