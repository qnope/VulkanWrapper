#pragma once

#include <algorithm>
#include <optional>

namespace r3d {
std::optional<int> index_of(const auto &range, const auto &object) {
    auto it = std::find(std::begin(range), std::end(range), object);
    if (it == std::end(range))
        return std::nullopt;
    return std::distance(std::begin(range), it);
}

std::optional<int> index_if(const auto &range, const auto &predicate) {
    auto it = std::find_if(std::begin(range), std::end(range), predicate);
    if (it == std::end(range))
        return std::nullopt;
    return std::distance(std::begin(range), it);
}
} // namespace r3d
