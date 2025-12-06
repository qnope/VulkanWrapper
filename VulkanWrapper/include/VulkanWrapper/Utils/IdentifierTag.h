#pragma once
#include "VulkanWrapper/3rd_party.h"
#include <typeindex>

namespace vw {
template <typename Tag> class IdentifierTag {
    friend class ::std::hash<IdentifierTag>;

  public:
    constexpr IdentifierTag(std::type_index index) noexcept
        : m_index{index} {}

    std::strong_ordering operator<=>(const IdentifierTag &) const = default;

  private:
    std::type_index m_index;
};
} // namespace vw

template <typename Tag> struct std::hash<vw::IdentifierTag<Tag>> {
    [[nodiscard]] std::size_t
    operator()(vw::IdentifierTag<Tag> tag) const noexcept {
        return tag.m_index.hash_code();
    }
};
