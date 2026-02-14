#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/Scene.h"
#include "VulkanWrapper/Pipeline/MeshRenderer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Utils/Error.h"
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
 * @brief Functional G-Buffer fill pass with lazy image allocation and buffer
 * reference materials
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

    using FragmentShaderMap =
        std::unordered_map<vw::Model::Material::MaterialTypeTag, std::string>;

    ColorPass(std::shared_ptr<vw::Device> device,
              std::shared_ptr<vw::Allocator> allocator,
              vw::Model::Material::BindlessMaterialManager &material_manager,
              FragmentShaderMap fragment_shaders,
              vk::Format depth_format = vk::Format::eD32Sfloat,
              std::span<const vk::Format> color_formats = default_color_formats)
        : Subpass(std::move(device), std::move(allocator))
        , m_color_formats(color_formats.begin(), color_formats.end())
        , m_material_manager(material_manager)
        , m_fragment_shaders(std::move(fragment_shaders))
        , m_descriptor_pool(create_descriptor_pool(depth_format)) {}

    /**
     * @brief Execute the G-Buffer fill pass
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

        // Request material resources (storage buffers + textures)
        for (const auto &resource : m_material_manager.get_resources()) {
            tracker.request(resource);
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

        // Bind uniform buffer descriptor set (set 0)
        auto uniform_descriptor_handle = descriptor_set.handle();

        // Draw all mesh instances grouped by material type
        vw::Model::Material::MaterialTypeTag current_tag{0xFFFFFFFF};

        for (const auto &instance : scene.instances()) {
            auto material_type = instance.mesh.material_type_tag();

            // Bind pipeline and descriptor sets when material type changes
            if (material_type != current_tag) {
                current_tag = material_type;
                auto pipeline = m_mesh_renderer->pipeline_for(material_type);
                if (!pipeline) {
                    continue;
                }
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                 pipeline->handle());
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       pipeline->layout().handle(), 0,
                                       uniform_descriptor_handle, nullptr);

                // Bind additional descriptor set if handler provides one
                auto *h = m_material_manager.handler(material_type);
                if (h) {
                    if (auto ds = h->additional_descriptor_set()) {
                        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                               pipeline->layout().handle(), 1,
                                               *ds, nullptr);
                    }
                }
            }

            auto pipeline = m_mesh_renderer->pipeline_for(material_type);
            if (pipeline) {
                instance.mesh.draw(cmd, pipeline->layout(), instance.transform);
            }
        }

        cmd.endRendering();

        return ColorPassOutput{.color = color.view,
                               .normal = normal.view,
                               .tangent = tangent.view,
                               .bitangent = bitangent.view,
                               .position = position.view};
    }

  private:
    vw::DescriptorPool create_descriptor_pool(vk::Format depth_format) {
        // Create descriptor layout for uniform buffer
        m_descriptor_layout =
            vw::DescriptorSetLayoutBuilder(m_device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex |
                                         vk::ShaderStageFlagBits::eFragment,
                                     1)
                .build();

        // Create mesh renderer with pipelines
        m_mesh_renderer = create_renderer(depth_format);

        return vw::DescriptorPoolBuilder(m_device, m_descriptor_layout).build();
    }

    std::shared_ptr<vw::MeshRenderer> create_renderer(vk::Format depth_format) {
        auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
            m_device, "Shaders/GBuffer/gbuffer.spv");

        auto push_constant_range =
            vk::PushConstantRange()
                .setStageFlags(vk::ShaderStageFlagBits::eVertex |
                               vk::ShaderStageFlagBits::eFragment)
                .setOffset(0)
                .setSize(sizeof(vw::Model::MeshPushConstants));

        auto renderer = std::make_shared<vw::MeshRenderer>();

        for (auto [tag, handler] : m_material_manager.handlers()) {
            auto it = m_fragment_shaders.find(tag);

            if (it == m_fragment_shaders.end()) {
                throw vw::LogicException::invalid_state(
                    "The tag has no fragment shader associated");
            }

            auto fragment =
                vw::ShaderModule::create_from_spirv_file(m_device, it->second);

            auto layout_builder =
                vw::PipelineLayoutBuilder(m_device)
                    .with_descriptor_set_layout(m_descriptor_layout)
                    .with_push_constant_range(push_constant_range);

            auto extra_layout = handler->additional_descriptor_set_layout();
            if (extra_layout) {
                layout_builder.with_descriptor_set_layout(
                    std::move(extra_layout));
            }

            vw::GraphicsPipelineBuilder builder(m_device,
                                                layout_builder.build());
            builder.add_vertex_binding<vw::FullVertex3D>()
                .add_shader(vk::ShaderStageFlagBits::eVertex, vertex_shader)
                .add_shader(vk::ShaderStageFlagBits::eFragment, fragment)
                .with_dynamic_viewport_scissor()
                .with_depth_test(false, vk::CompareOp::eEqual)
                .set_depth_format(depth_format);

            for (auto format : m_color_formats) {
                builder.add_color_attachment(format);
            }

            renderer->add_pipeline(tag, builder.build());
        }

        return renderer;
    }

    std::vector<vk::Format> m_color_formats;
    vw::Model::Material::BindlessMaterialManager &m_material_manager;
    FragmentShaderMap m_fragment_shaders;

    // Resources (order matters!)
    std::shared_ptr<vw::DescriptorSetLayout> m_descriptor_layout;
    std::shared_ptr<vw::MeshRenderer> m_mesh_renderer;
    vw::DescriptorPool m_descriptor_pool;
};
