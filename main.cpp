#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/PhysicalDevice.h>
#include <VulkanWrapper/Vulkan/Queue.h>
#include <VulkanWrapper/Vulkan/Surface.h>
#include <VulkanWrapper/Vulkan/Swapchain.h>
#include <VulkanWrapper/Window/SDL_Initializer.h>
#include <VulkanWrapper/Window/Window.h>

constexpr std::string_view COLOR = "COLOR";

std::vector<vw::ImageView> create_image_views(const vw::Device &device,
                                              const vw::Swapchain &swapchain) {
    std::vector<vw::ImageView> result;
    for (auto &&image : swapchain.images()) {
        auto imageView = vw::ImageViewBuilder(device, image)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();
        result.push_back(std::move(imageView));
    }
    return result;
}

std::vector<vw::Framebuffer>
createFramebuffers(vw::Device &device, const vw::RenderPass &renderPass,
                   const vw::Swapchain &swapchain,
                   const std::vector<vw::ImageView> &images) {
    std::vector<vw::Framebuffer> framebuffers;

    for (const auto &imageView : images) {
        auto framebuffer =
            vw::FramebufferBuilder(device, renderPass, swapchain.width(),
                                   swapchain.height())
                .add_attachment(imageView)
                .build();
        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}

void record(vk::CommandBuffer commandBuffer, vk::Extent2D extent,
            const vw::Framebuffer &framebuffer, const vw::Pipeline &pipeline,
            const vw::RenderPass &renderPass,
            const vw::Buffer<vw::ColoredVertex2D, true> &buffer) {
    vw::CommandBufferRecorder(commandBuffer)
        .begin_render_pass(renderPass, framebuffer)
        .bind_graphics_pipeline(pipeline)
        .bind_vertex_buffer(0, buffer)
        .draw(3, 1, 0, 0);
}

int main() {
    try {
        const std::vector<vw::ColoredVertex2D> vertices = {
            {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

        vw::SDL_Initializer initializer;
        vw::Window window = vw::WindowBuilder(initializer)
                                .with_title("Coucou")
                                .sized(800, 600)
                                .build();

        vw::Instance instance =
            vw::InstanceBuilder()
                .addPortability()
                .addExtensions(window.get_required_instance_extensions())
                .build();

        auto surface = window.create_surface(instance);

        auto device = instance.findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics |
                                      vk::QueueFlagBits::eCompute |
                                      vk::QueueFlagBits::eTransfer)
                          .with_presentation(surface.handle())
                          .with_synchronization_2()
                          .build();

        auto allocator = vw::AllocatorBuilder(instance, device).build();

        auto vertex_buffer =
            allocator.allocate_vertex_buffer<vw::ColoredVertex2D, true>(
                vertices.size());

        vertex_buffer.copy(vertices);

        auto swapchain = window.create_swapchain(device, surface.handle());

        auto vertexShader = vw::ShaderModule::create_from_spirv_file(
            device, "../../Shaders/bin/vert.spv");

        auto fragmentShader = vw::ShaderModule::create_from_spirv_file(
            device, "../../Shaders/bin/frag.spv");

        auto pipelineLayout = vw::PipelineLayoutBuilder(device).build();

        const auto attachment =
            vw::AttachmentBuilder(COLOR)
                .with_format(swapchain.format())
                .with_final_layout(vk::ImageLayout::ePresentSrcKHR)
                .build();

        auto subpass = vw::SubpassBuilder()
                           .add_color_attachment(
                               attachment, vk::ImageLayout::eAttachmentOptimal)
                           .build();

        auto renderPass = vw::RenderPassBuilder(device)
                              .add_subpass(std::move(subpass))
                              .build();

        auto pipeline =
            vw::GraphicsPipelineBuilder(device, renderPass)
                .add_vertex_binding<vw::ColoredVertex2D>()
                .add_shader(vk::ShaderStageFlagBits::eVertex,
                            std::move(vertexShader))
                .add_shader(vk::ShaderStageFlagBits::eFragment,
                            std::move(fragmentShader))
                .with_fixed_scissor(swapchain.width(), swapchain.height())
                .with_fixed_viewport(swapchain.width(), swapchain.height())
                .with_pipeline_layout(pipelineLayout)
                .add_color_attachment()
                .build();

        auto commandPool = vw::CommandPoolBuilder(device).build();
        auto image_views = create_image_views(device, swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        const auto framebuffers =
            createFramebuffers(device, renderPass, swapchain, image_views);

        const vk::Extent2D extent(swapchain.width(), swapchain.height());

        for (auto [framebuffer, commandBuffer] :
             std::views::zip(framebuffers, commandBuffers))
            record(commandBuffer, extent, framebuffer, pipeline, renderPass,
                   vertex_buffer);

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(device).build();

        while (!window.is_close_requested()) {
            window.update();

            auto index = swapchain.acquire_next_image(imageAvailableSemaphore);

            const vk::PipelineStageFlags waitStage =
                vk::PipelineStageFlagBits::eTopOfPipe;

            const auto image_available_handle =
                imageAvailableSemaphore.handle();
            const auto render_finished_handle =
                renderFinishedSemaphore.handle();

            device.graphicsQueue().enqueue_command_buffer(
                commandBuffers[index]);

            auto fence = device.graphicsQueue().submit(
                {&waitStage, 1}, {&image_available_handle, 1},
                {&render_finished_handle, 1});

            device.presentQueue().present(swapchain, index,
                                          renderFinishedSemaphore);
        }

        device.wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
