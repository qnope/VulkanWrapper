#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"

namespace vw::Model::Material {
class MaterialManager {
  public:
    MaterialManager(DescriptorPool pool);
    MaterialManager(const MaterialManager &) = delete;
    MaterialManager(MaterialManager &&) = delete;

    MaterialManager &operator=(const MaterialManager &) = delete;
    MaterialManager &operator=(MaterialManager &&) = delete;

    virtual ~MaterialManager() = default;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout() const noexcept;

  protected:
    [[nodiscard]] vk::DescriptorSet
    allocate_set(const DescriptorAllocator &allocator);

  private:
    DescriptorPool m_descriptor_pool;
};

template <const MaterialTypeTag *> class ConcreteMaterialManager;
} // namespace vw::Model::Material
