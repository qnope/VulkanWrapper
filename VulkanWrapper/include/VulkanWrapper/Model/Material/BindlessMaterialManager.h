#pragma once
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"
#include "VulkanWrapper/Model/Material/IMaterialTypeHandler.h"
#include "VulkanWrapper/Model/Material/Material.h"
#include <memory>
#include <unordered_map>

struct aiMaterial;

namespace vw::Model::Material {

class BindlessMaterialManager {
  public:
    BindlessMaterialManager(std::shared_ptr<const Device> device,
                            std::shared_ptr<Allocator> allocator,
                            std::shared_ptr<StagingBufferManager> staging);

    template <typename Handler> void register_handler() {
        auto handler = Handler::template create<Handler>(m_device, m_allocator,
                                                         m_texture_manager);
        auto tag = handler->tag();
        m_handlers[tag] = std::move(handler);
        m_sorted_handlers.clear();
    }

    [[nodiscard]] Material
    create_material(const aiMaterial *mat,
                    const std::filesystem::path &base_path);

    [[nodiscard]] BindlessTextureManager &texture_manager() noexcept;
    [[nodiscard]] const BindlessTextureManager &
    texture_manager() const noexcept;

    [[nodiscard]] IMaterialTypeHandler *handler(MaterialTypeTag tag);
    [[nodiscard]] const IMaterialTypeHandler *
    handler(MaterialTypeTag tag) const;

    void upload_all();

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

  private:
    void ensure_sorted_handlers() const;

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    std::shared_ptr<StagingBufferManager> m_staging;

    BindlessTextureManager m_texture_manager;

    std::unordered_map<MaterialTypeTag, std::unique_ptr<IMaterialTypeHandler>>
        m_handlers;
    mutable std::vector<IMaterialTypeHandler *> m_sorted_handlers;
};

} // namespace vw::Model::Material
