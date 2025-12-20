#pragma once
#include "VulkanWrapper/3rd_party.h"
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
    return Container(std::forward<Range>(range).begin(),
                     std::forward<Range>(range).end());
}

template <typename T> struct construct_t {};

template <typename T> constexpr construct_t<T> construct{};

template <typename Range, typename T>
auto operator|(Range &&range, construct_t<T> /*unused*/) {
    auto constructor = [](const auto &x) {
        if constexpr (requires { T{x}; })
            return T{x};
        else
            return std::apply(
                [](auto &&...xs) {
                    return T{std::forward<decltype(xs)>(xs)...};
                },
                x);
    };
    return std::forward<Range>(range) | std::views::transform(constructor);
}
} // namespace vw
