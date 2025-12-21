#pragma once
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferUsage.h"
#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"
#include "VulkanWrapper/Model/Material/IMaterialTypeHandler.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::Model::Material {

template <typename GpuData>
class MaterialTypeHandler : public IMaterialTypeHandler {
  public:
    using SSBOType = Buffer<GpuData, true, StorageBufferUsage>;

    template <typename Derived>
    static std::unique_ptr<Derived>
    create(std::shared_ptr<const Device> device,
           std::shared_ptr<Allocator> allocator,
           BindlessTextureManager &texture_manager) {
        static_assert(std::is_base_of_v<MaterialTypeHandler, Derived>,
                      "Derived must inherit from MaterialTypeHandler");
        auto handler = std::unique_ptr<Derived>(new Derived(
            std::move(device), std::move(allocator), texture_manager));
        handler->create_layout();
        handler->create_descriptor_pool();
        handler->allocate_descriptor_set();
        return handler;
    }

    [[nodiscard]] std::optional<Material>
    try_create(const aiMaterial *mat,
               const std::filesystem::path &base_path) final {
        auto data = try_create_gpu_data(mat, base_path);
        if (!data) {
            return std::nullopt;
        }
        uint32_t index = static_cast<uint32_t>(m_material_data.size());
        m_material_data.push_back(*data);
        m_dirty = true;
        return Material{tag(), index};
    }

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout() const final {
        return m_layout;
    }

    [[nodiscard]] vk::DescriptorSet descriptor_set() const final {
        return m_descriptor_set.handle();
    }

    void upload() final {
        if (!m_dirty || m_material_data.empty()) {
            return;
        }

        auto size_bytes =
            static_cast<VkDeviceSize>(m_material_data.size() * sizeof(GpuData));
        m_ssbo.emplace(m_allocator->allocate_buffer(
            size_bytes, true, vk::BufferUsageFlags(StorageBufferUsage),
            vk::SharingMode::eExclusive));
        m_ssbo->write(m_material_data, 0);

        update_descriptors();
        m_dirty = false;
    }

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_resources() const final {
        std::vector<Barrier::ResourceState> resources;

        if (m_ssbo) {
            resources.push_back(Barrier::BufferState{
                .buffer = m_ssbo->handle(),
                .offset = 0,
                .size = m_ssbo->size_bytes(),
                .stage = vk::PipelineStageFlagBits2::eVertexShader |
                         vk::PipelineStageFlagBits2::eFragmentShader |
                         vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                .access = vk::AccessFlagBits2::eShaderStorageRead});
        }

        auto texture_resources = get_texture_resources();
        resources.insert(resources.end(), texture_resources.begin(),
                         texture_resources.end());

        return resources;
    }

  protected:
    MaterialTypeHandler(std::shared_ptr<const Device> device,
                        std::shared_ptr<Allocator> allocator,
                        BindlessTextureManager &texture_manager)
        : m_device{std::move(device)}
        , m_allocator{std::move(allocator)}
        , m_texture_manager{texture_manager} {}

    [[nodiscard]] virtual std::optional<GpuData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) = 0;

    virtual void setup_layout(DescriptorSetLayoutBuilder &builder) = 0;

    virtual void setup_descriptors(DescriptorAllocator &alloc) = 0;

    [[nodiscard]] virtual std::vector<Barrier::ResourceState>
    get_texture_resources() const {
        return {};
    }

    [[nodiscard]] BindlessTextureManager &texture_manager() noexcept {
        return m_texture_manager;
    }

    [[nodiscard]] const BindlessTextureManager &
    texture_manager() const noexcept {
        return m_texture_manager;
    }

    void add_ssbo_descriptor(DescriptorAllocator &alloc, int binding) {
        if (m_ssbo) {
            alloc.add_storage_buffer(
                binding, m_ssbo->handle(), 0, m_ssbo->size_bytes(),
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderStorageRead);
        }
    }

  private:
    void create_layout() {
        DescriptorSetLayoutBuilder builder(m_device);
        setup_layout(builder);
        m_layout = builder.build();
    }

    void create_descriptor_pool() {
        m_pool.emplace(DescriptorPoolBuilder(m_device, m_layout)
                           .with_update_after_bind()
                           .build());
    }

    void allocate_descriptor_set() {
        m_descriptor_set = m_pool->allocate_set();
    }

    void update_descriptors() {
        DescriptorAllocator alloc;
        setup_descriptors(alloc);
        m_pool->update_set(m_descriptor_set.handle(), alloc);
    }

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    BindlessTextureManager &m_texture_manager;

    std::shared_ptr<DescriptorSetLayout> m_layout;
    std::optional<DescriptorPool> m_pool;
    DescriptorSet m_descriptor_set{{}, {}};

    std::vector<GpuData> m_material_data;
    std::optional<SSBOType> m_ssbo;
    bool m_dirty = false;
};

} // namespace vw::Model::Material
