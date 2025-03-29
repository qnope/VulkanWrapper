#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"

#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/Material.h"

namespace vw::Model::Material {
static vw::DescriptorPool create_pool(const Device &device) {
    auto layout =
        DescriptorSetLayoutBuilder(device)
            .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
            .build();
    return DescriptorPoolBuilder(device, layout).build();
}

ConcreteMaterialManager<&textured_material_tag>::ConcreteMaterialManager(
    const Device &device, StagingBufferManager &staging_buffer) noexcept
    : MaterialManager{create_pool(device)}
    , m_staging_buffer{&staging_buffer} {}

Material ConcreteMaterialManager<&textured_material_tag>::allocate(
    const std::filesystem::path &path) {
    auto image = m_staging_buffer->stage_image_from_path(path, true);

    DescriptorAllocator set_allocator;
    set_allocator.add_combined_image(0, image);

    auto set = allocate_set(set_allocator);
    m_combined_images.push_back(std::move(image));
    return {textured_material_tag, set};
}
} // namespace vw::Model::Material
