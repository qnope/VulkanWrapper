#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include <filesystem>

namespace vw {

class BufferBase;

namespace rt {
class RayTracedScene;
} // namespace rt

/**
 * @brief Depth pre-pass (Z-Pass) with lazy image allocation
 *
 * Renders scene geometry into a depth-only attachment for use by
 * subsequent passes. The depth image is lazily allocated on first
 * execute() and cached for reuse.
 *
 * The UBO and scene are provided via setters before execute().
 */
class ZPass : public RenderPass {
  public:
    ZPass(std::shared_ptr<Device> device,
          std::shared_ptr<Allocator> allocator,
          const std::filesystem::path &shader_dir,
          vk::Format depth_format = vk::Format::eD32Sfloat);

    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override;
    std::string_view name() const override { return "ZPass"; }

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker &tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    /// Set the uniform buffer containing view/projection matrices
    void set_uniform_buffer(const BufferBase &ubo);

    /// Set the scene containing mesh instances to render
    void set_scene(const rt::RayTracedScene &scene);

  private:
    vk::Format m_depth_format;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    DescriptorPool m_descriptor_pool;

    const BufferBase *m_uniform_buffer = nullptr;
    const rt::RayTracedScene *m_scene = nullptr;
};

} // namespace vw
