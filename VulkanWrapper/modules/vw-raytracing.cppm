module;
#include "VulkanWrapper/3rd_party.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

export module vw:raytracing;
import :core;
import :utils;
import :vulkan;
import :command;
import :memory;
import :pipeline;
import :model;

export namespace vw::rt {

// ---- GeometryReference ----

#pragma pack(push, 1)
struct GeometryReference {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
    int32_t vertex_offset;
    int32_t first_index;
    uint32_t material_type;
    uint32_t _padding = 0;
    uint64_t material_address;
    glm::mat4 matrix;
};
#pragma pack(pop)

static_assert(sizeof(GeometryReference) == 104,
              "GeometryReference must be 104 bytes for GPU scalar layout");

using GeometryReferenceBuffer =
    Buffer<GeometryReference, true, StorageBufferUsage>;

// ---- Acceleration Structure types ----

namespace as {

using AccelerationStructureBuffer =
    Buffer<std::byte, false,
           VkBufferUsageFlags(
               vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
               vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

using ScratchBuffer =
    Buffer<std::byte, false,
           VkBufferUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer |
                              vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

using InstanceBuffer = Buffer<
    vk::AccelerationStructureInstanceKHR, true,
    VkBufferUsageFlags(
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

// ---- BottomLevelAccelerationStructure ----

class BottomLevelAccelerationStructure
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
    BottomLevelAccelerationStructure(
        vk::UniqueAccelerationStructureKHR acceleration_structure,
        vk::DeviceAddress address);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept;

  private:
    vk::DeviceAddress m_device_address;
};

class BottomLevelAccelerationStructureList {
  public:
    BottomLevelAccelerationStructureList(
        std::shared_ptr<const Device> device,
        std::shared_ptr<const Allocator> allocator);

    using AccelerationStructureBufferList = BufferList<
        std::byte, false,
        VkBufferUsageFlags(
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

    using ScratchBufferList =
        BufferList<std::byte, false,
                   VkBufferUsageFlags(
                       vk::BufferUsageFlagBits::eStorageBuffer |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

    AccelerationStructureBufferList::BufferInfo
    allocate_acceleration_structure_buffer(vk::DeviceSize size);
    ScratchBufferList::BufferInfo allocate_scratch_buffer(vk::DeviceSize size);

    BottomLevelAccelerationStructure &
    add(BottomLevelAccelerationStructure &&blas);
    [[nodiscard]] std::vector<vk::DeviceAddress> device_addresses() const;

    vk::CommandBuffer command_buffer();
    void submit_and_wait();

  private:
    AccelerationStructureBufferList m_acceleration_structure_buffer_list;
    ScratchBufferList m_scratch_buffer_list;
    std::vector<BottomLevelAccelerationStructure>
        m_all_bottom_level_acceleration_structure;

    CommandPool m_command_pool;
    vk::CommandBuffer m_command_buffer;
    std::shared_ptr<const Device> m_device;
};

class BottomLevelAccelerationStructureBuilder {
  public:
    BottomLevelAccelerationStructureBuilder(
        std::shared_ptr<const Device> device);

    BottomLevelAccelerationStructureBuilder &
    add_geometry(const vk::AccelerationStructureGeometryKHR &geometry,
                 const vk::AccelerationStructureBuildRangeInfoKHR &offset);

    BottomLevelAccelerationStructureBuilder &
    add_mesh(const Model::Mesh &mesh);

    BottomLevelAccelerationStructure &
    build_into(BottomLevelAccelerationStructureList &list);

  private:
    std::shared_ptr<const Device> m_device;
    std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> m_ranges;
};

// ---- TopLevelAccelerationStructure ----

class TopLevelAccelerationStructure
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
    TopLevelAccelerationStructure(
        vk::UniqueAccelerationStructureKHR acceleration_structure,
        vk::DeviceAddress address, AccelerationStructureBuffer buffer);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept;

  private:
    vk::DeviceAddress m_device_address;
    AccelerationStructureBuffer m_buffer;
};

class TopLevelAccelerationStructureBuilder {
  public:
    TopLevelAccelerationStructureBuilder(
        std::shared_ptr<const Device> device,
        std::shared_ptr<const Allocator> allocator);

    TopLevelAccelerationStructureBuilder &
    add_bottom_level_acceleration_structure_address(
        vk::DeviceAddress address, const glm::mat4 &transform,
        uint32_t custom_index = 0, uint32_t sbt_record_offset = 0);

    TopLevelAccelerationStructure build(vk::CommandBuffer command_buffer);

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    std::vector<vk::AccelerationStructureInstanceKHR> m_instances;
};

} // namespace as

// ---- RayTracingPipeline ----

constexpr uint64_t ShaderBindingTableHandleSizeAlignment = 64;
using ShaderBindingTableHandle = std::vector<std::byte>;

class RayTracingPipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    friend class RayTracingPipelineBuilder;

    RayTracingPipeline(std::shared_ptr<const Device> device,
                       vk::UniquePipeline pipeline,
                       PipelineLayout pipeline_layout,
                       uint32_t number_miss_shader,
                       uint32_t number_close_hit_shader);

  public:
    [[nodiscard]] const PipelineLayout &layout() const noexcept;

    [[nodiscard]] ShaderBindingTableHandle ray_generation_handle() const;
    [[nodiscard]] std::span<const ShaderBindingTableHandle>
    miss_handles() const;
    [[nodiscard]] std::span<const ShaderBindingTableHandle>
    closest_hit_handles() const;

    [[nodiscard]] vk::PipelineLayout handle_layout() const;

  private:
    PipelineLayout m_layout;
    uint32_t m_number_miss_shader;
    uint32_t m_number_close_hit_shader;
    std::vector<ShaderBindingTableHandle> m_handles;
};

class RayTracingPipelineBuilder {
  public:
    RayTracingPipelineBuilder(std::shared_ptr<const Device> device,
                              std::shared_ptr<const Allocator> allocator,
                              PipelineLayout pipelineLayout);

    RayTracingPipelineBuilder &
    set_ray_generation_shader(std::shared_ptr<const ShaderModule> module);

    RayTracingPipelineBuilder &
    add_closest_hit_shader(std::shared_ptr<const ShaderModule> module);

    RayTracingPipelineBuilder &
    add_miss_shader(std::shared_ptr<const ShaderModule> module);

    RayTracingPipeline build();

  private:
    [[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo>
    create_stages() const;
    [[nodiscard]] std::vector<vk::RayTracingShaderGroupCreateInfoKHR>
    create_groups() const;

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    PipelineLayout m_pipelineLayout;

    std::shared_ptr<const ShaderModule> m_ray_generation_shader;
    std::vector<std::shared_ptr<const ShaderModule>> m_miss_shaders;
    std::vector<std::shared_ptr<const ShaderModule>> m_closest_hit_shaders;
};

// ---- ShaderBindingTable ----

constexpr uint64_t ShaderBindingTableHandleRecordSize = 256;

constexpr uint64_t MaximumRecordInShaderBindingTable = 4'096;

constexpr auto ShaderBindingTableUsage =
    VkBufferUsageFlags(vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress);

struct alignas(ShaderBindingTableHandleSizeAlignment) ShaderBindingTableRecord {
    ShaderBindingTableRecord(std::span<const std::byte> handle) {
        std::ranges::copy(handle, data.begin());
    }

    ShaderBindingTableRecord(std::span<const std::byte> handle,
                             const auto &object) {
        static_assert(sizeof(object) + ShaderBindingTableHandleSizeAlignment <
                      ShaderBindingTableHandleRecordSize);
        std::ranges::copy(handle, data.begin());
        std::memcpy(data.data() + handle.size(), &object, sizeof(object));
    }

    std::array<std::byte, ShaderBindingTableHandleRecordSize> data{};
};

class ShaderBindingTable {
  public:
    ShaderBindingTable(std::shared_ptr<const Allocator> allocator,
                       const ShaderBindingTableHandle &raygen_handle);

    void add_miss_record(const ShaderBindingTableHandle &handle,
                         const auto &...object) {
        ShaderBindingTableRecord record{handle, object...};
        m_sbt_ray_generation_and_miss_buffer.write(record,
                                                   m_number_raygen_miss++);
    }

    void add_hit_record(const ShaderBindingTableHandle &handle,
                        const auto &...object) {
        ShaderBindingTableRecord record{handle, object...};
        m_sbt_closest_hit_buffer.write(record, m_number_hit++);
    }

    [[nodiscard]] vk::StridedDeviceAddressRegionKHR raygen_region() const;
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR miss_region() const;
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR hit_region() const;

  private:
    std::shared_ptr<const Allocator> m_allocator;

    uint64_t m_number_raygen_miss = 0;
    uint64_t m_number_hit = 0;

    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_ray_generation_and_miss_buffer;

    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_closest_hit_buffer;
};

// ---- RayTracedScene ----

struct InstanceId {
    uint32_t value;
    bool operator==(const InstanceId &other) const noexcept = default;
};

class RayTracedScene {
  public:
    RayTracedScene(std::shared_ptr<const Device> device,
                   std::shared_ptr<const Allocator> allocator);

    RayTracedScene(const RayTracedScene &) = delete;
    RayTracedScene &operator=(const RayTracedScene &) = delete;
    RayTracedScene(RayTracedScene &&) = default;
    RayTracedScene &operator=(RayTracedScene &&) = default;

    [[nodiscard]] InstanceId
    add_instance(const Model::Mesh &mesh,
                 const glm::mat4 &transform = glm::mat4(1.0f));

    void set_transform(InstanceId instance_id, const glm::mat4 &transform);
    [[nodiscard]] const glm::mat4 &get_transform(InstanceId instance_id) const;

    void set_visible(InstanceId instance_id, bool visible);
    [[nodiscard]] bool is_visible(InstanceId instance_id) const;

    void set_sbt_offset(InstanceId instance_id, uint32_t offset);
    [[nodiscard]] uint32_t get_sbt_offset(InstanceId instance_id) const;

    void set_custom_index(InstanceId instance_id, uint32_t custom_index);
    [[nodiscard]] uint32_t get_custom_index(InstanceId instance_id) const;

    void remove_instance(InstanceId instance_id);
    [[nodiscard]] bool is_valid(InstanceId instance_id) const;

    void build();
    void update();

    [[nodiscard]] bool needs_build() const noexcept;
    [[nodiscard]] bool needs_update() const noexcept;

    [[nodiscard]] vk::DeviceAddress tlas_device_address() const;
    [[nodiscard]] vk::AccelerationStructureKHR tlas_handle() const;
    [[nodiscard]] const as::TopLevelAccelerationStructure &tlas() const;

    [[nodiscard]] vk::DeviceAddress geometry_buffer_address() const;
    [[nodiscard]] const GeometryReferenceBuffer &geometry_buffer() const;
    [[nodiscard]] bool has_geometry_buffer() const noexcept;

    [[nodiscard]] size_t mesh_count() const noexcept;
    [[nodiscard]] size_t instance_count() const noexcept;
    [[nodiscard]] size_t visible_instance_count() const noexcept;

    [[nodiscard]] const Model::Scene &scene() const noexcept {
        return m_scene;
    }
    [[nodiscard]] Model::Scene &scene() noexcept { return m_scene; }

    void set_material_sbt_mapping(
        std::unordered_map<Model::Material::MaterialTypeTag, uint32_t>
            mapping);

  private:
    struct Instance {
        uint32_t blas_index;
        glm::mat4 transform = glm::mat4(1.0f);
        bool visible = true;
        bool active = true;
        uint32_t sbt_offset = 0;
        uint32_t custom_index = 0;
    };

    struct MeshGeometry {
        std::shared_ptr<const Model::FullVertex3DBuffer> full_vertex_buffer;
        std::shared_ptr<const IndexBuffer> index_buffer;
        int vertex_offset;
        int first_index;
        Model::Material::Material material;
        glm::mat4 matrix;
    };

    uint32_t get_or_create_blas_index(const Model::Mesh &mesh);
    void build_blas();
    void build_tlas();
    void build_geometry_buffer();

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;

    std::vector<Instance> m_instances;

    std::optional<as::BottomLevelAccelerationStructureList> m_blas_list;
    std::optional<as::TopLevelAccelerationStructure> m_tlas;

    bool m_blas_dirty = false;
    bool m_tlas_dirty = false;

    std::unordered_map<Model::Mesh, uint32_t> m_mesh_to_blas_index;
    std::vector<MeshGeometry> m_mesh_geometries;
    std::optional<GeometryReferenceBuffer> m_geometry_buffer;

    Model::Scene m_scene;

    std::optional<
        std::unordered_map<Model::Material::MaterialTypeTag, uint32_t>>
        m_material_sbt_mapping;
};

} // namespace vw::rt
