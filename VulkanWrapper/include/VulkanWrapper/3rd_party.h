#pragma once

// ---- Vulkan HPP configuration macros ----
namespace vk::detail {
class DispatchLoaderDynamic;
}

namespace vw {
const vk::detail::DispatchLoaderDynamic &DefaultDispatcher();
}

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VULKAN_HPP_ASSERT_ON_RESULT

#ifndef VW_LIB
#define VULKAN_HPP_DEFAULT_DISPATCHER vw::DefaultDispatcher()
#endif

// ---- GLM configuration macros ----
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// ---- Material type macros (cannot be exported from modules) ----
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

template <> struct std::hash<vw::Model::Material::MaterialTypeTag> {
    [[nodiscard]] std::size_t
    operator()(vw::Model::Material::MaterialTypeTag tag) const noexcept {
        return std::hash<vw::Model::Material::MaterialTypeId>{}(tag.id());
    }
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define VW_DECLARE_MATERIAL_TYPE(Name)                                         \
    extern const ::vw::Model::Material::MaterialTypeTag Name

#define VW_DEFINE_MATERIAL_TYPE(Name)                                          \
    const ::vw::Model::Material::MaterialTypeTag Name {                        \
        ::vw::Model::Material::detail::g_next_material_type_id++               \
    }
// NOLINTEND(cppcoreguidelines-macro-usage)
