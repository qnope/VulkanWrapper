#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"

#include "VulkanWrapper/Utils/Error.h"
#include <algorithm>
#include <ranges>

namespace vw::Model::Material {

BindlessMaterialManager::BindlessMaterialManager(
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator,
    std::shared_ptr<StagingBufferManager> staging)
    : m_device{std::move(device)}
    , m_allocator{std::move(allocator)}
    , m_staging{std::move(staging)}
    , m_texture_manager{m_device, m_allocator, m_staging} {}

Material BindlessMaterialManager::create_material(
    const aiMaterial *mat, const std::filesystem::path &base_path) {
    ensure_sorted_handlers();

    for (auto *handler : m_sorted_handlers) {
        if (auto material = handler->try_create(mat, base_path)) {
            return *material;
        }
    }

    throw LogicException::invalid_state(
        "No material handler can create material from the given data");
}

BindlessTextureManager &BindlessMaterialManager::texture_manager() noexcept {
    return m_texture_manager;
}

const BindlessTextureManager &
BindlessMaterialManager::texture_manager() const noexcept {
    return m_texture_manager;
}

IMaterialTypeHandler *BindlessMaterialManager::handler(MaterialTypeTag tag) {
    auto it = m_handlers.find(tag);
    return it != m_handlers.end() ? it->second.get() : nullptr;
}

const IMaterialTypeHandler *
BindlessMaterialManager::handler(MaterialTypeTag tag) const {
    auto it = m_handlers.find(tag);
    return it != m_handlers.end() ? it->second.get() : nullptr;
}

void BindlessMaterialManager::upload_all() {
    for (auto &[tag, handler] : m_handlers) {
        handler->upload();
    }
}

std::vector<Barrier::ResourceState>
BindlessMaterialManager::get_resources() const {
    std::vector<Barrier::ResourceState> resources;

    for (const auto &[tag, handler] : m_handlers) {
        auto handler_resources = handler->get_resources();
        resources.insert(resources.end(), handler_resources.begin(),
                         handler_resources.end());
    }

    return resources;
}

void BindlessMaterialManager::ensure_sorted_handlers() const {
    if (!m_sorted_handlers.empty()) {
        return;
    }

    for (const auto &[tag, handler] : m_handlers) {
        m_sorted_handlers.push_back(handler.get());
    }

    std::ranges::sort(m_sorted_handlers, [](const auto *a, const auto *b) {
        return static_cast<int>(a->priority()) >
               static_cast<int>(b->priority());
    });
}

} // namespace vw::Model::Material
