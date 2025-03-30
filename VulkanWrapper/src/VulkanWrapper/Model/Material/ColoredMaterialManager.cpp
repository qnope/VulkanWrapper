#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Internal/MaterialInfo.h"
#include "VulkanWrapper/Model/Material/Material.h"

namespace vw::Model::Material {

static DescriptorPool create_pool(const Device &device) {
    auto layout =
        DescriptorSetLayoutBuilder(device)
            .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment, 1)
            .build();
    return DescriptorPoolBuilder(device, layout).build();
}

ConcreteMaterialManager<&colored_material_tag>::ConcreteMaterialManager(
    const Device &device, const Allocator &allocator,
    StagingBufferManager &staging_buffer_manager) noexcept
    : MaterialManager(create_pool(device))
    , m_staging_buffer_manager{&staging_buffer_manager}
    , m_buffer{allocator} {}

Material ConcreteMaterialManager<&colored_material_tag>::allocate(
    const glm::vec4 &color) noexcept {
    auto [buffer, offset] = m_buffer.create_buffer(sizeof(color));
    m_staging_buffer_manager->fill_buffer(std::span(&color, 1), *buffer,
                                          offset);

    DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer->handle(),
                                 offset * sizeof(glm::vec4), sizeof(glm::vec4));
    auto set = allocate_set(allocator);
    return {.material_type = colored_material_tag, .descriptor_set = set};
}

std::optional<Material>
allocate_colored_material(const Internal::MaterialInfo &info,
                          ColoredMaterialManager &manager) {
    if (info.diffuse_color)
        return manager.allocate(*info.diffuse_color);
    return manager.allocate(glm::vec4{});
}

} // namespace vw::Model::Material
