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

class BindlessTextureManager;

class IMaterialTypeHandler {
  public:
    virtual ~IMaterialTypeHandler() = default;

    [[nodiscard]] virtual MaterialTypeTag tag() const = 0;
    [[nodiscard]] virtual MaterialPriority priority() const = 0;

    [[nodiscard]] virtual std::optional<Material>
    try_create(const aiMaterial *mat,
               const std::filesystem::path &base_path) = 0;

    [[nodiscard]] virtual std::shared_ptr<const DescriptorSetLayout>
    layout() const = 0;

    [[nodiscard]] virtual vk::DescriptorSet descriptor_set() const = 0;

    virtual void upload() = 0;

    [[nodiscard]] virtual std::vector<Barrier::ResourceState>
    get_resources() const = 0;
};

} // namespace vw::Model::Material
