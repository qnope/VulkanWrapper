#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/Material/MaterialPriority.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include <filesystem>
#include <optional>

struct aiMaterial;

namespace vw::Model::Material {

class IMaterialTypeHandler {
  public:
    virtual ~IMaterialTypeHandler() = default;

    [[nodiscard]] virtual MaterialTypeTag tag() const = 0;
    [[nodiscard]] virtual MaterialPriority priority() const = 0;

    [[nodiscard]] virtual std::optional<Material>
    try_create(const aiMaterial *mat,
               const std::filesystem::path &base_path) = 0;

    [[nodiscard]] virtual vk::DeviceAddress buffer_address() const = 0;
    [[nodiscard]] virtual uint32_t stride() const = 0;

    virtual void upload() = 0;

    [[nodiscard]] virtual std::vector<Barrier::ResourceState>
    get_resources() const = 0;

    [[nodiscard]] virtual std::optional<vk::DescriptorSet>
    additional_descriptor_set() const {
        return std::nullopt;
    }

    [[nodiscard]] virtual std::shared_ptr<const DescriptorSetLayout>
    additional_descriptor_set_layout() const {
        return nullptr;
    }
};

} // namespace vw::Model::Material
