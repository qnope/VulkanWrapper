#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"

#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::Model::Material {

static constexpr uint32_t MAX_MATERIALS = 4096;

BindlessMaterialManager::BindlessMaterialManager(
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator,
    std::shared_ptr<StagingBufferManager> staging)
    : m_device{device}
    , m_allocator{allocator}
    , m_staging{staging}
    , m_texture_manager{device, allocator, staging} {

    create_textured_layout();
    create_colored_layout();
    create_textured_descriptor_pool();
    create_colored_descriptor_pool();
    allocate_descriptor_sets();
}

void BindlessMaterialManager::create_textured_layout() {
    // binding 0: sampler
    // binding 1: storage_buffer (SSBO for material data)
    // binding 2: sampled_images (bindless)
    m_textured_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_sampler(vk::ShaderStageFlagBits::eFragment)
            .with_storage_buffer_bindless(vk::ShaderStageFlagBits::eFragment)
            .with_sampled_images_bindless(vk::ShaderStageFlagBits::eFragment,
                                          BindlessTextureManager::MAX_TEXTURES)
            .build();
}

void BindlessMaterialManager::create_colored_layout() {
    m_colored_layout =
        DescriptorSetLayoutBuilder(m_device)
            .with_storage_buffer(vk::ShaderStageFlagBits::eFragment)
            .build();
}

void BindlessMaterialManager::create_textured_descriptor_pool() {
    std::vector pool_sizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage,
                               BindlessTextureManager::MAX_TEXTURES),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)};

    auto pool_info =
        vk::DescriptorPoolCreateInfo()
            .setMaxSets(1)
            .setPoolSizes(pool_sizes)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);

    m_textured_pool =
        check_vk(m_device->handle().createDescriptorPoolUnique(pool_info),
                 "Failed to create textured descriptor pool");
}

void BindlessMaterialManager::create_colored_descriptor_pool() {
    std::vector pool_sizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)};

    auto pool_info =
        vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(pool_sizes);

    m_colored_pool =
        check_vk(m_device->handle().createDescriptorPoolUnique(pool_info),
                 "Failed to create colored descriptor pool");
}

void BindlessMaterialManager::allocate_descriptor_sets() {
    {
        vk::DescriptorSetLayout layout_handle = m_textured_layout->handle();
        auto alloc_info = vk::DescriptorSetAllocateInfo()
                              .setDescriptorPool(m_textured_pool.get())
                              .setSetLayouts(layout_handle);

        auto sets =
            check_vk(m_device->handle().allocateDescriptorSets(alloc_info),
                     "Failed to allocate textured descriptor set");
        m_textured_descriptor_set = sets[0];
    }

    {
        vk::DescriptorSetLayout layout_handle = m_colored_layout->handle();
        auto alloc_info = vk::DescriptorSetAllocateInfo()
                              .setDescriptorPool(m_colored_pool.get())
                              .setSetLayouts(layout_handle);

        auto sets =
            check_vk(m_device->handle().allocateDescriptorSets(alloc_info),
                     "Failed to allocate colored descriptor set");
        m_colored_descriptor_set = sets[0];
    }
}

Material BindlessMaterialManager::create_textured_material(
    const std::filesystem::path &path) {
    uint32_t texture_index = m_texture_manager.register_texture(path);

    TexturedMaterialData data{};
    data.diffuse_texture_index = texture_index;

    uint32_t material_index = static_cast<uint32_t>(m_textured_data.size());
    m_textured_data.push_back(data);
    m_dirty = true;

    return {.material_type = bindless_textured_material_tag,
            .material_index = material_index};
}

Material
BindlessMaterialManager::create_colored_material(const glm::vec4 &color) {
    ColoredMaterialData data{};
    data.color = color;

    uint32_t material_index = static_cast<uint32_t>(m_colored_data.size());
    m_colored_data.push_back(data);
    m_dirty = true;

    return {.material_type = bindless_colored_material_tag,
            .material_index = material_index};
}

vk::Buffer BindlessMaterialManager::textured_materials_ssbo() const noexcept {
    return m_textured_ssbo ? m_textured_ssbo->handle() : vk::Buffer{};
}

vk::Buffer BindlessMaterialManager::colored_materials_ssbo() const noexcept {
    return m_colored_ssbo ? m_colored_ssbo->handle() : vk::Buffer{};
}

BindlessTextureManager &BindlessMaterialManager::texture_manager() noexcept {
    return m_texture_manager;
}

const BindlessTextureManager &
BindlessMaterialManager::texture_manager() const noexcept {
    return m_texture_manager;
}

std::shared_ptr<const DescriptorSetLayout>
BindlessMaterialManager::textured_layout() const noexcept {
    return m_textured_layout;
}

std::shared_ptr<const DescriptorSetLayout>
BindlessMaterialManager::colored_layout() const noexcept {
    return m_colored_layout;
}

vk::DescriptorSet
BindlessMaterialManager::textured_descriptor_set() const noexcept {
    return m_textured_descriptor_set;
}

vk::DescriptorSet
BindlessMaterialManager::colored_descriptor_set() const noexcept {
    return m_colored_descriptor_set;
}

void BindlessMaterialManager::upload_materials() {
    if (!m_dirty) {
        return;
    }

    if (!m_textured_data.empty()) {
        m_textured_ssbo.emplace(
            create_buffer<TexturedSSBO>(*m_allocator, m_textured_data.size()));
        m_staging->fill_buffer(
            std::span<const TexturedMaterialData>(m_textured_data),
            *m_textured_ssbo, 0);
        update_textured_descriptors();
    }

    if (!m_colored_data.empty()) {
        m_colored_ssbo.emplace(
            create_buffer<ColoredSSBO>(*m_allocator, m_colored_data.size()));
        m_staging->fill_buffer(
            std::span<const ColoredMaterialData>(m_colored_data),
            *m_colored_ssbo, 0);
        update_colored_descriptors();
    }

    m_dirty = false;
}

void BindlessMaterialManager::update_textured_descriptors() {
    std::vector<vk::WriteDescriptorSet> writes;

    vk::DescriptorImageInfo sampler_info;
    sampler_info.setSampler(m_texture_manager.sampler());

    auto sampler_write = vk::WriteDescriptorSet()
                             .setDstSet(m_textured_descriptor_set)
                             .setDstBinding(0)
                             .setDescriptorCount(1)
                             .setDescriptorType(vk::DescriptorType::eSampler)
                             .setImageInfo(sampler_info);
    writes.push_back(sampler_write);

    auto ssbo_info = vk::DescriptorBufferInfo()
                         .setBuffer(m_textured_ssbo->handle())
                         .setOffset(0)
                         .setRange(VK_WHOLE_SIZE);

    auto ssbo_write = vk::WriteDescriptorSet()
                          .setDstSet(m_textured_descriptor_set)
                          .setDstBinding(1)
                          .setDescriptorCount(1)
                          .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                          .setBufferInfo(ssbo_info);

    writes.push_back(ssbo_write);
    m_device->handle().updateDescriptorSets(writes, nullptr);

    // Write texture image descriptors at binding 2
    m_texture_manager.write_image_descriptors(m_textured_descriptor_set, 2);
}

void BindlessMaterialManager::update_colored_descriptors() {
    auto ssbo_info = vk::DescriptorBufferInfo()
                         .setBuffer(m_colored_ssbo->handle())
                         .setOffset(0)
                         .setRange(VK_WHOLE_SIZE);

    auto ssbo_write = vk::WriteDescriptorSet()
                          .setDstSet(m_colored_descriptor_set)
                          .setDstBinding(0)
                          .setDescriptorCount(1)
                          .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                          .setBufferInfo(ssbo_info);

    m_device->handle().updateDescriptorSets(ssbo_write, nullptr);
}

std::vector<Barrier::ResourceState>
BindlessMaterialManager::get_resources() const {
    auto resources = m_texture_manager.get_resources();

    if (m_textured_ssbo) {
        Barrier::BufferState state;
        state.buffer = m_textured_ssbo->handle();
        state.offset = 0;
        state.size = VK_WHOLE_SIZE;
        state.stage = vk::PipelineStageFlagBits2::eFragmentShader;
        state.access = vk::AccessFlagBits2::eShaderStorageRead;
        resources.push_back(state);
    }

    if (m_colored_ssbo) {
        Barrier::BufferState state;
        state.buffer = m_colored_ssbo->handle();
        state.offset = 0;
        state.size = VK_WHOLE_SIZE;
        state.stage = vk::PipelineStageFlagBits2::eFragmentShader;
        state.access = vk::AccessFlagBits2::eShaderStorageRead;
        resources.push_back(state);
    }

    return resources;
}

} // namespace vw::Model::Material
