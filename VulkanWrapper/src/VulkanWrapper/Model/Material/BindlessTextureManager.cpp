#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"

#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::Model::Material {

BindlessTextureManager::BindlessTextureManager(
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator,
    std::shared_ptr<StagingBufferManager> staging)
    : m_device{std::move(device)}
    , m_allocator{std::move(allocator)}
    , m_staging{std::move(staging)}
    , m_layout{DescriptorSetLayoutBuilder(m_device)
                   .with_sampler(vk::ShaderStageFlagBits::eFragment)
                   .with_sampled_images_bindless(
                       vk::ShaderStageFlagBits::eFragment, MAX_TEXTURES)
                   .build()}
    , m_pool{DescriptorPoolBuilder(m_device, m_layout)
                 .with_update_after_bind()
                 .build()}
    , m_descriptor_set{m_pool.allocate_set()}
    , m_sampler{SamplerBuilder(m_device).build()} {

    DescriptorAllocator alloc;
    alloc.add_sampler(0, m_sampler->handle());
    m_pool.update_set(m_descriptor_set.handle(), alloc);
}

uint32_t
BindlessTextureManager::register_texture(const std::filesystem::path &path) {
    auto it = m_path_to_index.find(path);
    if (it != m_path_to_index.end()) {
        return it->second;
    }

    CombinedImage combined = m_staging->stage_image_from_path(path, true);

    uint32_t index = static_cast<uint32_t>(m_combined_images.size());

    DescriptorAllocator alloc;
    alloc.add_sampled_image(1, combined.image_view_ptr(),
                            vk::PipelineStageFlagBits2::eFragmentShader,
                            vk::AccessFlagBits2::eShaderSampledRead, index);

    m_pool.update_set(m_descriptor_set.handle(), alloc);

    m_combined_images.push_back(std::move(combined));
    m_path_to_index[path] = index;
    return index;
}

vk::DescriptorSet BindlessTextureManager::descriptor_set() const noexcept {
    return m_descriptor_set.handle();
}

std::shared_ptr<const DescriptorSetLayout>
BindlessTextureManager::layout() const noexcept {
    return m_layout;
}

uint32_t BindlessTextureManager::texture_count() const noexcept {
    return static_cast<uint32_t>(m_combined_images.size());
}

void BindlessTextureManager::update_descriptors() {
    m_last_updated_count = static_cast<uint32_t>(m_combined_images.size());
}

std::vector<Barrier::ResourceState>
BindlessTextureManager::get_resources() const {
    std::vector<Barrier::ResourceState> resources;
    for (const auto &combined : m_combined_images) {
        Barrier::ImageState state;
        state.image = combined.image();
        state.subresourceRange = combined.subresource_range();
        // Use eReadOnlyOptimal to match the layout after staging
        // (execute_image_barrier_transfer_dst_to_sampled and
        // execute_image_barrier_transfer_src_to_dst both use eReadOnlyOptimal)
        state.layout = vk::ImageLayout::eReadOnlyOptimal;
        state.stage = vk::PipelineStageFlagBits2::eFragmentShader;
        state.access = vk::AccessFlagBits2::eShaderSampledRead;
        resources.push_back(state);
    }
    return resources;
}

vk::Sampler BindlessTextureManager::sampler() const noexcept {
    return m_sampler->handle();
}

void BindlessTextureManager::write_image_descriptors(
    vk::DescriptorSet dest_set, uint32_t dest_binding) const {
    if (m_combined_images.empty()) {
        return;
    }

    std::vector<vk::DescriptorImageInfo> image_infos;
    image_infos.reserve(m_combined_images.size());

    for (const auto &combined : m_combined_images) {
        vk::DescriptorImageInfo info;
        info.setImageView(combined.image_view())
            .setImageLayout(vk::ImageLayout::eReadOnlyOptimal);
        image_infos.push_back(info);
    }

    auto image_write =
        vk::WriteDescriptorSet()
            .setDstSet(dest_set)
            .setDstBinding(dest_binding)
            .setDstArrayElement(0)
            .setDescriptorCount(static_cast<uint32_t>(image_infos.size()))
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setImageInfo(image_infos);

    m_device->handle().updateDescriptorSets(image_write, nullptr);
}

} // namespace vw::Model::Material
