#include <view.hpp>

namespace ecs {

size_t view_range::size() const noexcept
{
    return types.size();
}

void view_range::push_back(
    size_t entity, std::span<void *> ptrs)
{
    views.reserve(views.size() + ptrs.size() + 1);
    views.push_back(reinterpret_cast<void *>(entity));
    views.insert(views.end(), ptrs.rbegin(), ptrs.rend());
}

void view_range::erase(size_t entity)
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

bool view_range::captures(
    const component_set &comps) const noexcept
{
    if (comps.size() < types.size())
        return false;

    for (auto hash : types) {
        if (!comps.contains({ hash, 0 }))
            return false;
    }

    return true;
}

} // namespace ecs

