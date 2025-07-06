#include "Application.h"
#include "ColorPass.h"
#include "SkyPass.h"
#include "TonemapPass.h"
#include "ZPass.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/AccelerationStructure/AccelerationStructure.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Image/ImageLoader.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Model/Importer.h>
#include <VulkanWrapper/Model/Material/ColoredMaterialManager.h>
#include <VulkanWrapper/Model/Material/TexturedMaterialManager.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Pipeline/MeshRenderer.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/RenderPass/Attachment.h>
#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Queue.h>

std::vector<std::shared_ptr<const vw::ImageView>>
create_image_views(const vw::Device &device, const vw::Swapchain &swapchain) {
    std::vector<std::shared_ptr<const vw::ImageView>> result;
    for (auto &&image : swapchain.images()) {
        auto imageView = vw::ImageViewBuilder(device, image)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();
        result.push_back(std::move(imageView));
    }
    return result;
}

struct UBOData {
    glm::mat4 proj = [] {
        auto proj = glm::perspective(glm::radians(60.0F), 1600.0F / 900.0F, 1.F,
                                     10000.0F);
        proj[1][1] *= -1;
        return proj;
    }();
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0F, 300.0F, 0.0F),
                    glm::vec3(1.0F, 300.5F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    glm::mat4 model = glm::mat4(1.0);
};

vw::Buffer<UBOData, true, vw::UniformBufferUsage>
createUbo(vw::Allocator &allocator) {
    auto buffer =
        allocator.create_buffer<UBOData, true, vw::UniformBufferUsage>(1);
    UBOData data;
    buffer.copy({&data, 1}, 0);
    return buffer;
}

std::vector<vw::Framebuffer> createFramebuffers(
    vw::Device &device, const vw::Allocator &allocator,
    const vw::RenderPass &renderPass, const vw::Swapchain &swapchain,
    const std::vector<std::shared_ptr<const vw::ImageView>> &images,
    const std::shared_ptr<const vw::ImageView> &depth_buffer) {
    std::vector<vw::Framebuffer> framebuffers;

    auto create_img = [&]() {
        return allocator.create_image_2D(
            swapchain.width(), swapchain.height(), false,
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eInputAttachment);
    };

    auto create_img_view = [&](auto img) {
        return vw::ImageViewBuilder(device, img)
            .setImageType(vk::ImageViewType::e2D)
            .build();
    };

    for (const auto &imageView : images) {
        auto img_color = allocator.create_image_2D(
            swapchain.width(), swapchain.height(), false,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eInputAttachment);

        auto img_position = create_img();
        auto img_normal = create_img();
        auto img_tangeant = create_img();
        auto img_biTangeant = create_img();
        auto img_light = create_img();

        auto img_view_color = create_img_view(img_color);
        auto img_view_position = create_img_view(img_position);
        auto img_view_normal = create_img_view(img_normal);
        auto img_view_tangeant = create_img_view(img_tangeant);
        auto img_view_biTangeant = create_img_view(img_biTangeant);
        auto img_view_light = create_img_view(img_light);

        auto framebuffer =
            vw::FramebufferBuilder(device, renderPass, swapchain.width(),
                                   swapchain.height())
                .add_attachment(img_view_color)
                .add_attachment(img_view_position)
                .add_attachment(img_view_normal)
                .add_attachment(img_view_tangeant)
                .add_attachment(img_view_biTangeant)
                .add_attachment(img_view_light)
                .add_attachment(imageView)
                .add_attachment(depth_buffer)
                .build();
        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}

using namespace glm;

std::ostream &operator<<(std::ostream &s, const vec3 obj) {
    s << "(" << obj.x << ", " << obj.y << ", " << obj.z << ")";
    return s;
}

int main() {
    try {
        App app;
        auto descriptor_set_layout =
            vw::DescriptorSetLayoutBuilder(app.device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                .build();

        auto uniform_buffer = createUbo(app.allocator);

        auto descriptor_pool =
            vw::DescriptorPoolBuilder(app.device, descriptor_set_layout)
                .build();

        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(0, uniform_buffer.handle(), 0,
                                                uniform_buffer.size_bytes());

        auto descriptor_set =
            descriptor_pool.allocate_set(descriptor_allocator);

        const auto depth_buffer = app.allocator.create_image_2D(
            app.swapchain.width(), app.swapchain.height(), false,
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);

        const auto depth_buffer_view =
            vw::ImageViewBuilder(app.device, depth_buffer)
                .setImageType(vk::ImageViewType::e2D)
                .build();

        vw::Model::MeshManager mesh_manager(app.device, app.allocator);
        mesh_manager.read_file("../../../Models/Sponza/sponza.obj");
        mesh_manager.read_file("../../../Models/cube.obj");

        // --- Acceleration Structure Creation ---
        // Build BLAS from all meshes
        vw::AccelerationStructure::BottomLevelAccelerationStructureBuilder
            blasBuilder(app.device, app.allocator);
        blasBuilder.add_geometries(mesh_manager.meshes());
        auto blas = std::move(blasBuilder).build();

        // Build TLAS from the BLAS (single instance, identity transform)
        vw::AccelerationStructure::TopLevelAccelerationStructureBuilder
            tlasBuilder(app.device, app.allocator);
        tlasBuilder.add_instance(blas);
        auto tlas = std::move(tlasBuilder).build();
        // --- End Acceleration Structure Creation ---

        const auto color_attachment =
            vw::AttachmentBuilder{}
                .with_format(vk::Format::eR8G8B8A8Unorm)
                .with_final_layout(vk::ImageLayout::eAttachmentOptimal)
                .build();

        const auto data_attachment =
            vw::AttachmentBuilder{}
                .with_format(vk::Format::eR32G32B32A32Sfloat)
                .with_final_layout(vk::ImageLayout::eAttachmentOptimal)
                .build();

        const auto final_attachment =
            vw::AttachmentBuilder{}
                .with_format(app.swapchain.format())
                .with_final_layout(vk::ImageLayout::ePresentSrcKHR)
                .build();

        const auto depth_attachment =
            vw::AttachmentBuilder{}
                .with_format(depth_buffer->format())
                .with_final_layout(
                    vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .build();

        auto depth_subpass = std::make_unique<ZPass>(
            app.device, mesh_manager, descriptor_set_layout,
            app.swapchain.width(), app.swapchain.height(), descriptor_set);
        auto color_subpass = std::make_unique<ColorSubpass>(
            app.device, mesh_manager, descriptor_set_layout,
            app.swapchain.width(), app.swapchain.height(), descriptor_set);
        auto sky_pass = std::make_unique<SkyPass>(
            app.device, app.allocator, app.swapchain.width(),
            app.swapchain.height(), UBOData{}.proj, UBOData{}.view);
        auto sky_buffer = sky_pass->get_ubo();
        auto tonemap_pass = std::make_unique<TonemapPass>(
            app.device, app.swapchain.width(), app.swapchain.height());

        auto renderPass =
            vw::RenderPassBuilder(app.device)
                .add_attachment(color_attachment,
                                vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f))
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment,
                                vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f))
                .add_attachment(final_attachment, vk::ClearColorValue())
                .add_attachment(depth_attachment,
                                vk::ClearDepthStencilValue(1.0))
                .add_subpass(z_pass_tag, std::move(depth_subpass))
                .add_subpass(color_pass_tag, std::move(color_subpass))
                .add_subpass(sky_pass_tag, std::move(sky_pass))
                .add_subpass(tonemap_pass_tag, std::move(tonemap_pass))
                .add_dependency(z_pass_tag, color_pass_tag)
                .add_dependency(z_pass_tag, sky_pass_tag)
                .add_dependency(sky_pass_tag, tonemap_pass_tag)
                .add_dependency(color_pass_tag, tonemap_pass_tag)
                .build();

        auto commandPool = vw::CommandPoolBuilder(app.device).build();
        auto image_views = create_image_views(app.device, app.swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        const auto framebuffers =
            createFramebuffers(app.device, app.allocator, renderPass,
                               app.swapchain, image_views, depth_buffer_view);

        const vk::Extent2D extent(uint32_t(app.swapchain.width()),
                                  uint32_t(app.swapchain.height()));

        for (auto [framebuffer, commandBuffer] :
             std::views::zip(framebuffers, commandBuffers)) {
            vw::CommandBufferRecorder recorder(commandBuffer);
            renderPass.execute(commandBuffer, framebuffer);
        }

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(app.device).build();

        auto cmd_buffer = mesh_manager.fill_command_buffer();

        app.device.graphicsQueue().enqueue_command_buffer(cmd_buffer);

        float angle = -10.0;
        while (!app.window.is_close_requested()) {
            app.window.update();

            angle += 0.1;

            if (angle > 360)
                angle = 0.0;

            std::cout << angle << std::endl;

            SkyPass::UBO ubo{UBOData{}.proj, UBOData{}.view, angle};
            sky_buffer->copy({&ubo, 1u}, 0);

            auto index =
                app.swapchain.acquire_next_image(imageAvailableSemaphore);

            const vk::PipelineStageFlags waitStage =
                vk::PipelineStageFlagBits::eTopOfPipe;

            const auto image_available_handle =
                imageAvailableSemaphore.handle();
            const auto render_finished_handle =
                renderFinishedSemaphore.handle();

            app.device.graphicsQueue().enqueue_command_buffer(
                commandBuffers[index]);

            auto fence = app.device.graphicsQueue().submit(
                {&waitStage, 1}, {&image_available_handle, 1},
                {&render_finished_handle, 1});

            app.device.presentQueue().present(app.swapchain, index,
                                              renderFinishedSemaphore);
            app.device.wait_idle();
        }

        app.device.wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << '\n';
    }
}
