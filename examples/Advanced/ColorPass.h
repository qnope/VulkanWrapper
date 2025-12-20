#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialManager.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/Pipeline/MeshRenderer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include <span>

enum class ColorPassSlot { Color, Normal, Tangent, Bitangent, Position };

/**
 * @brief Output structure for the ColorPass G-Buffer
 */
struct ColorPassOutput {
    std::shared_ptr<const vw::ImageView> color;
    std::shared_ptr<const vw::ImageView> normal;
    std::shared_ptr<const vw::ImageView> tangent;
    std::shared_ptr<const vw::ImageView> bitangent;
    std::shared_ptr<const vw::ImageView> position;
};

/**
 * @brief Functional G-Buffer fill pass with lazy image allocation
 *
 * This pass lazily allocates its G-Buffer images on first execute() call.
 * Images are cached by (width, height, frame_index) and reused on subsequent
 * calls.
 */
class ColorPass : public vw::Subpass<ColorPassSlot> {
  public:
    static constexpr std::array<vk::Format, 5> default_color_formats = {
        vk::Format::eR8G8B8A8Unorm,      // color
        vk::Format::eR32G32B32A32Sfloat, // normal
        vk::Format::eR32G32B32A32Sfloat, // tangent
        vk::Format::eR32G32B32A32Sfloat, // bitangent
        vk::Format::eR32G32B32A32Sfloat  // position
    };

    ColorPass(std::shared_ptr<vw::Device> device,
              std::shared_ptr<vw::Allocator> allocator,
              const vw::Model::MeshManager &mesh_manager,
              vk::Format depth_format = vk::Format::eD32Sfloat,
              std::span<const vk::Format> color_formats = default_color_formats)
        : Subpass(std::move(device), std::move(allocator))
        , m_color_formats(color_formats.begin(), color_formats.end())
        , m_descriptor_pool(
              create_descriptor_pool(mesh_manager, depth_format)) {}

    /**
     * @brief Execute the G-Buffer fill pass
     *
     * @param cmd Command buffer to record into
     * @param tracker Resource tracker for barrier management
     * @param width Render width
     * @param height Render height
     * @param frame_index Frame index for multi-buffering
     * @param scene Scene containing mesh instances to render
     * @param depth_view Depth buffer from Z-Pass (used for depth testing)
     * @param uniform_buffer Uniform buffer containing view/projection matrices
     * @return ColorPassOutput containing all G-Buffer image views
     */
    template <typename UBOType>
    ColorPassOutput
    execute(vk::CommandBuffer cmd, vw::Barrier::ResourceTracker &tracker,
            vw::Width width, vw::Height height, size_t frame_index,
            const vw::Model::Scene &scene,
            std::shared_ptr<const vw::ImageView> depth_view,
            const vw::Buffer<UBOType, true, vw::UniformBufferUsage>
                &uniform_buffer) {

        constexpr auto usageFlags = vk::ImageUsageFlagBits::eColorAttachment |
                                    vk::ImageUsageFlagBits::eInputAttachment |
                                    vk::ImageUsageFlagBits::eSampled |
                                    vk::ImageUsageFlagBits::eTransferSrc;

        // Lazy allocation of 5 G-Buffer images
        const auto &color =
            get_or_create_image(ColorPassSlot::Color, width, height,
                                frame_index, m_color_formats[0], usageFlags);
        const auto &normal =
            get_or_create_image(ColorPassSlot::Normal, width, height,
                                frame_index, m_color_formats[1], usageFlags);
        const auto &tangent =
            get_or_create_image(ColorPassSlot::Tangent, width, height,
                                frame_index, m_color_formats[2], usageFlags);
        const auto &bitangent =
            get_or_create_image(ColorPassSlot::Bitangent, width, height,
                                frame_index, m_color_formats[3], usageFlags);
        const auto &position =
            get_or_create_image(ColorPassSlot::Position, width, height,
                                frame_index, m_color_formats[4], usageFlags);

        vk::Extent2D extent{static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height)};

        // Create descriptor set with uniform buffer
        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(
            0, uniform_buffer.handle(), 0, uniform_buffer.size_bytes(),
            vk::PipelineStageFlagBits2::eVertexShader |
                vk::PipelineStageFlagBits2::eFragmentShader,
            vk::AccessFlagBits2::eUniformRead);

        auto descriptor_set =
            m_descriptor_pool.allocate_set(descriptor_allocator);

        // Request resource states for barriers
        for (const auto &resource : descriptor_set.resources()) {
            tracker.request(resource);
        }

        // Request states for all output images
        std::array<const vw::CachedImage *, 5> cached_images = {
            &color, &normal, &tangent, &bitangent, &position};

        for (const auto *cached : cached_images) {
            tracker.request(vw::Barrier::ImageState{
                .image = cached->image->handle(),
                .subresourceRange = cached->view->subresource_range(),
                .layout = vk::ImageLayout::eColorAttachmentOptimal,
                .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .access = vk::AccessFlagBits2::eColorAttachmentWrite});
        }

        // Depth image for reading (from Z-Pass)
        tracker.request(vw::Barrier::ImageState{
            .image = depth_view->image()->handle(),
            .subresourceRange = depth_view->subresource_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead});

        // Request material resources
        for (const auto &instance : scene.instances()) {
            const auto &material_resources =
                instance.mesh.material().descriptor_set.resources();
            for (const auto &resource : material_resources) {
                tracker.request(resource);
            }
        }

        // Flush barriers
        tracker.flush(cmd);

        // Setup rendering attachments
        std::vector<vk::RenderingAttachmentInfo> color_attachments;
        for (const auto *cached : cached_images) {
            color_attachments.push_back(
                vk::RenderingAttachmentInfo()
                    .setImageView(cached->view->handle())
                    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setClearValue(
                        vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)));
        }

        vk::RenderingAttachmentInfo depth_attachment =
            vk::RenderingAttachmentInfo()
                .setImageView(depth_view->handle())
                .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore);

        vk::RenderingInfo rendering_info =
            vk::RenderingInfo()
                .setRenderArea(vk::Rect2D({0, 0}, extent))
                .setLayerCount(1)
                .setColorAttachments(color_attachments)
                .setPDepthAttachment(&depth_attachment);

        cmd.beginRendering(rendering_info);

        // Set viewport and scissor
        vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width),
                              static_cast<float>(extent.height), 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, extent);
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        // Draw all mesh instances
        auto descriptor_handle = descriptor_set.handle();
        std::span first_descriptor_sets = {&descriptor_handle, 1};
        for (const auto &instance : scene.instances()) {
            m_mesh_renderer->draw_mesh(
                cmd, instance.mesh, first_descriptor_sets, instance.transform);
        }

        cmd.endRendering();

        return ColorPassOutput{.color = color.view,
                               .normal = normal.view,
                               .tangent = tangent.view,
                               .bitangent = bitangent.view,
                               .position = position.view};
    }

  private:
    vw::DescriptorPool
    create_descriptor_pool(const vw::Model::MeshManager &mesh_manager,
                           vk::Format depth_format) {
        // Create descriptor layout for uniform buffer
        m_descriptor_layout =
            vw::DescriptorSetLayoutBuilder(m_device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex |
                                         vk::ShaderStageFlagBits::eFragment,
                                     1)
                .build();

        // Create mesh renderer with pipelines
        m_mesh_renderer = create_renderer(mesh_manager, depth_format);

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    std::shared_ptr<vw::MeshRenderer>
    create_renderer(const vw::Model::MeshManager &mesh_manager,
                    vk::Format depth_format) {
        auto vertexShader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/GBuffer/gbuffer.spv");
        auto fragment_textured = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/GBuffer/gbuffer_textured.spv");
        auto fragment_colored = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/GBuffer/gbuffer_colored.spv");

        auto create_pipeline =
            [&](std::shared_ptr<const vw::ShaderModule> fragment,
                std::shared_ptr<const vw::DescriptorSetLayout>
                    material_layout) {
                auto pipelineLayout =
                    vw::PipelineLayoutBuilder(m_device)
                        .with_descriptor_set_layout(m_descriptor_layout)
                        .with_descriptor_set_layout(material_layout)
                        .with_push_constant_range(
                            vk::PushConstantRange()
                                .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                                .setOffset(0)
                                .setSize(sizeof(glm::mat4)))
                        .build();

                vw::GraphicsPipelineBuilder builder(m_device,
                                                    std::move(pipelineLayout));
                builder.add_vertex_binding<vw::FullVertex3D>()
                    .add_shader(vk::ShaderStageFlagBits::eVertex, vertexShader)
                    .add_shader(vk::ShaderStageFlagBits::eFragment, fragment)
                    .with_dynamic_viewport_scissor()
                    .with_depth_test(false, vk::CompareOp::eEqual)
                    .set_depth_format(depth_format);

                for (auto format : m_color_formats) {
                    builder.add_color_attachment(format);
                }

                return builder.build();
            };

        auto textured_pipeline = create_pipeline(
            fragment_textured, mesh_manager.material_manager_map().layout(
                                   vw::Model::Material::textured_material_tag));

        auto colored_pipeline = create_pipeline(
            fragment_colored, mesh_manager.material_manager_map().layout(
                                  vw::Model::Material::colored_material_tag));

        auto renderer = std::make_shared<vw::MeshRenderer>();
        renderer->add_pipeline(vw::Model::Material::textured_material_tag,
                               std::move(textured_pipeline));
        renderer->add_pipeline(vw::Model::Material::colored_material_tag,
                               std::move(colored_pipeline));
        return renderer;
    }

    std::vector<vk::Format> m_color_formats;

    // Resources (order matters!)
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<vw::MeshRenderer> m_mesh_renderer;
    vw::DescriptorPool m_descriptor_pool;
};
