#pragma once

#include <algorithm>
#include <optional>

namespace vw {
std::optional<int> index_of(const auto &range, const auto &object) {
    auto it = std::find(std::begin(range), std::end(range), object);
    if (it == std::end(range)) {
        return std::nullopt;
    }
    return std::distance(std::begin(range), it);
}

std::optional<int> index_if(const auto &range, const auto &predicate) {
    auto it = std::find_if(std::begin(range), std::end(range), predicate);
    if (it == std::end(range)) {
        return std::nullopt;
    }
    return std::distance(std::begin(range), it);
}

template <template <typename... Ts> typename Container> struct to_t {};

template <template <typename... Ts> typename Container>
constexpr to_t<Container> to{};

template <typename Range, template <typename... Ts> typename Container>
auto operator|(Range &&range, to_t<Container> /*unused*/) {
    return Container(std::ranges::begin(range), std::ranges::end(range));
}

} // namespace vw
