#pragma once
#include <cstdint>
#include <functional>

namespace vw::Model::Material {

using MaterialTypeId = uint32_t;

namespace detail {
inline MaterialTypeId g_next_material_type_id = 0;
} // namespace detail

class MaterialTypeTag {
  public:
    constexpr explicit MaterialTypeTag(MaterialTypeId id) noexcept
        : m_id{id} {}

    [[nodiscard]] constexpr MaterialTypeId id() const noexcept { return m_id; }

    auto operator<=>(const MaterialTypeTag &) const = default;

  private:
    MaterialTypeId m_id;
};

} // namespace vw::Model::Material

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define VW_REGISTER_MATERIAL_TYPE(Name)                                        \
    inline const ::vw::Model::Material::MaterialTypeTag Name{                  \
        ::vw::Model::Material::detail::g_next_material_type_id++}
// NOLINTEND(cppcoreguidelines-macro-usage)

template <> struct std::hash<vw::Model::Material::MaterialTypeTag> {
    [[nodiscard]] std::size_t
    operator()(vw::Model::Material::MaterialTypeTag tag) const noexcept {
        return std::hash<vw::Model::Material::MaterialTypeId>{}(tag.id());
    }
};
