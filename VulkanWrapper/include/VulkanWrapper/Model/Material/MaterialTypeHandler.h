#pragma once
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferUsage.h"
#include "VulkanWrapper/Model/Material/IMaterialTypeHandler.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::Model::Material {

template <typename GpuData>
class MaterialTypeHandler : public IMaterialTypeHandler {
  public:
    using SSBOType = Buffer<GpuData, true, StorageBufferUsage>;

    template <typename Derived, typename... Args>
    static std::unique_ptr<Derived> create(std::shared_ptr<const Device> device,
                                           std::shared_ptr<Allocator> allocator,
                                           Args &&...args) {
        static_assert(std::is_base_of_v<MaterialTypeHandler, Derived>,
                      "Derived must inherit from MaterialTypeHandler");
        return std::unique_ptr<Derived>(
            new Derived(std::move(device), std::move(allocator),
                        std::forward<Args>(args)...));
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
        return Material{tag(),
                        m_ssbo.device_address() +
                            index * sizeof(GpuData)};
    }

    [[nodiscard]] vk::DeviceAddress buffer_address() const final {
        return m_ssbo.device_address();
    }

    [[nodiscard]] uint32_t stride() const final { return sizeof(GpuData); }

    void upload() final {
        if (!m_dirty || m_material_data.empty()) {
            return;
        }
        m_ssbo.write(m_material_data, 0);
        m_dirty = false;
    }

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_resources() const final {
        std::vector<Barrier::ResourceState> resources;

        resources.push_back(Barrier::BufferState{
            .buffer = m_ssbo.handle(),
            .offset = 0,
            .size = m_ssbo.size_bytes(),
            .stage = vk::PipelineStageFlagBits2::eVertexShader |
                     vk::PipelineStageFlagBits2::eFragmentShader |
                     vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            .access = vk::AccessFlagBits2::eShaderStorageRead});

        auto texture_resources = get_texture_resources();
        resources.insert(resources.end(), texture_resources.begin(),
                         texture_resources.end());

        return resources;
    }

  protected:
    MaterialTypeHandler(std::shared_ptr<const Device> device,
                        std::shared_ptr<Allocator> allocator)
        : m_device{std::move(device)}
        , m_allocator{std::move(allocator)}
        , m_ssbo{create_buffer<SSBOType>(*m_allocator, 1024)} {}

    [[nodiscard]] virtual std::optional<GpuData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) = 0;

    [[nodiscard]] virtual std::vector<Barrier::ResourceState>
    get_texture_resources() const {
        return {};
    }

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    SSBOType m_ssbo;

    std::vector<GpuData> m_material_data;
    bool m_dirty = false;
};

} // namespace vw::Model::Material
