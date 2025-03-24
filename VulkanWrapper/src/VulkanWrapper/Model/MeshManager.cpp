#include "VulkanWrapper/Model/MeshManager.h"

#include "VulkanWrapper/Model/Importer.h"

namespace vw::Model {
namespace {
std::shared_ptr<DescriptorSetLayout> create_layout(const Device &device) {
    return DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
        .build();
}

DescriptorPool create_pool(const Device &device,
                           const std::shared_ptr<DescriptorSetLayout> &layout) {
    return DescriptorPoolBuilder(device, layout, 1'000).build();
}
} // namespace

MeshManager::MeshManager(const Device &device, const Allocator &allocator)
    : m_staging_buffer_manager{device, allocator}
    , m_descriptor_set_layout{create_layout(device)}
    , m_descriptor_pool{create_pool(device, m_descriptor_set_layout)}
    , m_vertex_buffer{allocator}
    , m_index_buffer{allocator} {}

std::shared_ptr<DescriptorSetLayout> MeshManager::layout() const noexcept {
    return m_descriptor_set_layout;
}

void MeshManager::read_file(const std::filesystem::path &path) {
    Model::Importer importer{path, *this};
}

vk::CommandBuffer MeshManager::fill_command_buffer() {
    return m_staging_buffer_manager.fill_command_buffer();
}

const std::vector<Mesh> &MeshManager::meshes() const noexcept {
    return m_meshes;
}
} // namespace vw::Model
