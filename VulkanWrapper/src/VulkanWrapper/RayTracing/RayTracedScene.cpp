#include "VulkanWrapper/RayTracing/RayTracedScene.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <algorithm>

namespace vw::rt {

RayTracedScene::RayTracedScene(std::shared_ptr<const Device> device,
                               std::shared_ptr<const Allocator> allocator)
    : m_device(std::move(device))
    , m_allocator(std::move(allocator)) {}

uint32_t RayTracedScene::get_or_create_blas_index(const Model::Mesh &mesh) {
    auto it = m_mesh_to_blas_index.find(mesh);
    if (it != m_mesh_to_blas_index.end()) {
        return it->second;
    }

    uint32_t index = static_cast<uint32_t>(m_mesh_to_blas_index.size());
    m_mesh_to_blas_index.emplace(mesh, index);
    m_blas_dirty = true;

    m_mesh_geometries.push_back(MeshGeometry{
        .full_vertex_buffer = mesh.full_vertex_buffer(),
        .index_buffer = mesh.index_buffer(),
        .vertex_offset = mesh.vertex_offset(),
        .first_index = mesh.first_index(),
        .material = mesh.material()});

    return index;
}

InstanceId RayTracedScene::add_instance(const Model::Mesh &mesh,
                                        const glm::mat4 &transform) {
    uint32_t blas_index = get_or_create_blas_index(mesh);

    Instance instance{.blas_index = blas_index,
                      .transform = transform,
                      .visible = true,
                      .active = true,
                      .sbt_offset = 0,
                      .custom_index = 0};

    InstanceId id{static_cast<uint32_t>(m_instances.size())};
    m_instances.push_back(instance);
    m_tlas_dirty = true;

    m_scene.add_mesh_instance(mesh, transform);

    return id;
}

void RayTracedScene::set_transform(InstanceId instance_id,
                                   const glm::mat4 &transform) {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    instance.transform = transform;
    m_tlas_dirty = true;
}

const glm::mat4 &RayTracedScene::get_transform(InstanceId instance_id) const {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    const auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    return instance.transform;
}

void RayTracedScene::set_visible(InstanceId instance_id, bool visible) {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    if (instance.visible != visible) {
        instance.visible = visible;
        m_tlas_dirty = true;
    }
}

bool RayTracedScene::is_visible(InstanceId instance_id) const {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    const auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    return instance.visible;
}

void RayTracedScene::set_sbt_offset(InstanceId instance_id, uint32_t offset) {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    instance.sbt_offset = offset;
    m_tlas_dirty = true;
}

uint32_t RayTracedScene::get_sbt_offset(InstanceId instance_id) const {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    const auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    return instance.sbt_offset;
}

void RayTracedScene::set_custom_index(InstanceId instance_id,
                                      uint32_t custom_index) {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    instance.custom_index = custom_index;
    m_tlas_dirty = true;
}

uint32_t RayTracedScene::get_custom_index(InstanceId instance_id) const {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    const auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance has been removed");
    }

    return instance.custom_index;
}

void RayTracedScene::remove_instance(InstanceId instance_id) {
    if (instance_id.value >= m_instances.size()) {
        throw LogicException::out_of_range("instance id", instance_id.value,
                                           m_instances.size());
    }

    auto &instance = m_instances[instance_id.value];
    if (!instance.active) {
        throw LogicException::invalid_state("Instance already removed");
    }

    instance.active = false;
    instance.visible = false;
    m_tlas_dirty = true;
}

bool RayTracedScene::is_valid(InstanceId instance_id) const {
    if (instance_id.value >= m_instances.size()) {
        return false;
    }
    return m_instances[instance_id.value].active;
}

void RayTracedScene::build() {
    if (m_mesh_to_blas_index.empty()) {
        throw LogicException::invalid_state("No meshes registered");
    }

    build_blas();
    build_geometry_buffer();
    build_tlas();

    m_blas_dirty = false;
    m_tlas_dirty = false;
}

void RayTracedScene::update() {
    if (!m_blas_list.has_value()) {
        throw LogicException::invalid_state(
            "Must call build() before update()");
    }

    if (m_blas_dirty) {
        throw LogicException::invalid_state(
            "New meshes registered, must call build() instead of update()");
    }

    if (m_tlas_dirty) {
        build_tlas();
        m_tlas_dirty = false;
    }
}

bool RayTracedScene::needs_build() const noexcept { return m_blas_dirty; }

bool RayTracedScene::needs_update() const noexcept {
    return m_tlas_dirty && !m_blas_dirty;
}

vk::DeviceAddress RayTracedScene::tlas_device_address() const {
    if (!m_tlas.has_value()) {
        throw LogicException::invalid_state("TLAS not built yet");
    }
    return m_tlas->device_address();
}

vk::AccelerationStructureKHR RayTracedScene::tlas_handle() const {
    if (!m_tlas.has_value()) {
        throw LogicException::invalid_state("TLAS not built yet");
    }
    return m_tlas->handle();
}

const as::TopLevelAccelerationStructure &RayTracedScene::tlas() const {
    if (!m_tlas.has_value()) {
        throw LogicException::invalid_state("TLAS not built yet");
    }
    return *m_tlas;
}

size_t RayTracedScene::mesh_count() const noexcept {
    return m_mesh_to_blas_index.size();
}

size_t RayTracedScene::instance_count() const noexcept {
    return std::ranges::count_if(m_instances,
                                 [](const auto &inst) { return inst.active; });
}

size_t RayTracedScene::visible_instance_count() const noexcept {
    return std::ranges::count_if(m_instances, [](const auto &inst) {
        return inst.active && inst.visible;
    });
}

void RayTracedScene::build_blas() {
    m_blas_list.emplace(m_device, m_allocator);

    // Collect meshes with their indices, then sort by index to ensure correct
    // ordering
    using MeshEntry =
        std::pair<std::reference_wrapper<const Model::Mesh>, uint32_t>;
    std::vector<MeshEntry> sorted_meshes;
    sorted_meshes.reserve(m_mesh_to_blas_index.size());
    for (const auto &[mesh, index] : m_mesh_to_blas_index) {
        sorted_meshes.emplace_back(std::cref(mesh), index);
    }
    std::ranges::sort(sorted_meshes, {}, &MeshEntry::second);

    for (const auto &[mesh_ref, index] : sorted_meshes) {
        const Model::Mesh &mesh = mesh_ref.get();
        as::BottomLevelAccelerationStructureBuilder(m_device)
            .add_geometry(mesh.acceleration_structure_geometry(),
                          mesh.acceleration_structure_range_info())
            .build_into(*m_blas_list);
    }

    m_blas_list->submit_and_wait();
}

void RayTracedScene::build_tlas() {
    if (m_tlas.has_value()) {
        m_device->wait_idle();
        m_tlas.reset();
    }

    auto blas_addresses = m_blas_list->device_addresses();

    auto command_pool = CommandPoolBuilder(m_device).build();
    auto command_buffer = command_pool.allocate(1).front();

    std::ignore = command_buffer.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    as::TopLevelAccelerationStructureBuilder builder(m_device, m_allocator);

    uint32_t visible_count = 0;
    for (const auto &instance : m_instances) {
        if (!instance.active || !instance.visible) {
            continue;
        }

        builder.add_bottom_level_acceleration_structure_address(
            blas_addresses[instance.blas_index], instance.transform,
            instance.blas_index, instance.sbt_offset);
        ++visible_count;
    }

    if (visible_count > 0) {
        m_tlas = builder.build(command_buffer);
    }

    std::ignore = command_buffer.end();

    auto &queue = const_cast<Device &>(*m_device).graphicsQueue();
    queue.enqueue_command_buffer(command_buffer);
    queue.submit({}, {}, {});
    m_device->wait_idle();
}

void RayTracedScene::build_geometry_buffer() {
    if (m_mesh_geometries.empty()) {
        return;
    }

    m_geometry_buffer.emplace(
        create_buffer<GeometryReferenceBuffer>(*m_allocator,
                                               m_mesh_geometries.size()));

    std::vector<GeometryReference> references;
    references.reserve(m_mesh_geometries.size());

    for (const auto &geom : m_mesh_geometries) {
        GeometryReference ref{
            .vertex_buffer_address = geom.full_vertex_buffer->device_address(),
            .vertex_offset = geom.vertex_offset,
            .index_buffer_address = geom.index_buffer->device_address(),
            .first_index = geom.first_index,
            .material_type = geom.material.material_type.id(),
            .material_index = geom.material.material_index};
        references.push_back(ref);
    }

    m_geometry_buffer->write(references, 0);
}

vk::DeviceAddress RayTracedScene::geometry_buffer_address() const {
    if (!m_geometry_buffer.has_value()) {
        throw LogicException::invalid_state("Geometry buffer not built yet");
    }
    return m_geometry_buffer->device_address();
}

const GeometryReferenceBuffer &RayTracedScene::geometry_buffer() const {
    if (!m_geometry_buffer.has_value()) {
        throw LogicException::invalid_state("Geometry buffer not built yet");
    }
    return *m_geometry_buffer;
}

bool RayTracedScene::has_geometry_buffer() const noexcept {
    return m_geometry_buffer.has_value();
}

} // namespace vw::rt
