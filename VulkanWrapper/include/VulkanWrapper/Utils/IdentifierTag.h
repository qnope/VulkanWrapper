#pragma once
#include "VulkanWrapper/3rd_party.h"
#include <typeindex>

namespace vw {
template <typename Tag> class IdentifierTag {
  public:
    IdentifierTag(std::type_index index) noexcept
        : m_index{index} {}

    std::strong_ordering operator<=>(const IdentifierTag &) const = default;

    [[nodiscard]] std::size_t hash() const noexcept {
        return m_index.hash_code();
    }

  private:
    std::type_index m_index;
};
} // namespace vw

template <typename Tag> struct std::hash<vw::IdentifierTag<Tag>> {
    [[nodiscard]] std::size_t
    operator()(const vw::IdentifierTag<Tag>& tag) const noexcept {
        return tag.hash();
    }
};
