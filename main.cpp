#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/Framebuffer.h>
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

std::vector<vw::Framebuffer>
createFramebuffers(vw::Device &device, const vw::RenderPass &renderPass,
                   const vw::Swapchain &swapchain) {
    std::vector<vw::Framebuffer> framebuffers;

    for (const auto &imageView : swapchain.imageViews()) {
        auto framebuffer = vw::FramebufferBuilder(renderPass, swapchain.width(),
                                                  swapchain.height())
                               .addAttachment(imageView)
                               .build(device);
        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}

void record(vk::CommandBuffer commandBuffer, vk::Extent2D extent,
            const vw::Framebuffer &framebuffer, const vw::Pipeline &pipeline,
            const vw::RenderPass &renderPass) {
    commandBuffer.begin(vk::CommandBufferBeginInfo());
    vk::ClearValue color;

    auto renderPassBeginInfo =
        vk::RenderPassBeginInfo()
            .setRenderPass(renderPass.handle())
            .setFramebuffer(framebuffer.handle())
            .setRenderArea(vk::Rect2D(vk::Offset2D(), extent))
            .setClearValues(color);

    auto subpassBegin =
        vk::SubpassBeginInfo().setContents(vk::SubpassContents::eInline);

    commandBuffer.beginRenderPass2(renderPassBeginInfo, subpassBegin);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               pipeline.handle());
    // const vk::Rect2D scissor{vk::Offset2D(), extent};
    // const vk::Viewport viewport(0.0, 0.0, extent.width, extent.height, 0.0,
    //                             1.0);
    // commandBuffer.setScissor(0, scissor);
    // commandBuffer.setViewport(0, viewport);
    commandBuffer.draw(3, 1, 0, 0);

    auto subpassEnd = vk::SubpassEndInfo();
    commandBuffer.endRenderPass2(subpassEnd);

    commandBuffer.end();
}

int main() {
    try {
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

        auto surface = window.createSurface(instance);

        auto device = instance.findGpu()
                          .withQueue(vk::QueueFlagBits::eGraphics |
                                     vk::QueueFlagBits::eCompute |
                                     vk::QueueFlagBits::eTransfer)
                          .withPresentQueue(surface.handle())
                          .build();

        auto swapchain =
            window.createSwapchain(device, surface.handle()).build();

        auto vertexShader = vw::ShaderModule::createFromSpirVFile(
            device, "../../Shaders/bin/vert.spv");

        auto fragmentShader = vw::ShaderModule::createFromSpirVFile(
            device, "../../Shaders/bin/frag.spv");

        auto pipelineLayout = vw::PipelineLayoutBuilder().build(device);

        const auto attachment =
            vw::AttachmentBuilder(COLOR)
                .withFormat(swapchain.format())
                .withFinalLayout(vk::ImageLayout::ePresentSrcKHR)
                .build();

        auto subpass = vw::SubpassBuilder()
                           .addColorAttachment(
                               attachment, vk::ImageLayout::eAttachmentOptimal)
                           .build();

        auto renderPass = vw::RenderPassBuilder()
                              .addSubpass(vk::PipelineBindPoint::eGraphics,
                                          std::move(subpass))
                              .build(device);

        auto pipeline =
            vw::GraphicsPipelineBuilder(renderPass)
                .addShaderModule(vk::ShaderStageFlagBits::eVertex,
                                 std::move(vertexShader))
                .addShaderModule(vk::ShaderStageFlagBits::eFragment,
                                 std::move(fragmentShader))
                .withFixedViewport(swapchain.width(), swapchain.height())
                .withFixedScissor(swapchain.width(), swapchain.height())
                .withPipelineLayout(pipelineLayout)
                .addColorAttachment()
                .build(device);

        auto commandPool = vw::CommandPoolBuilder().build(device);
        auto commandBuffers =
            commandPool.allocate(swapchain.imageViews().size());

        const auto framebuffers =
            createFramebuffers(device, renderPass, swapchain);

        const vk::Extent2D extent(swapchain.width(), swapchain.height());

        for (auto [framebuffer, commandBuffer] :
             std::views::zip(framebuffers, commandBuffers))
            record(commandBuffer, extent, framebuffer, pipeline, renderPass);

        auto fence = vw::FenceBuilder().build(device);
        auto imageAvailableSemaphore = vw::SemaphoreBuilder().build(device);
        auto renderFinishedSemaphore = vw::SemaphoreBuilder().build(device);

        while (!window.is_close_requested()) {
            window.update();
            fence.wait();
            fence.reset();

            const auto acquireInfo =
                vk::AcquireNextImageInfoKHR()
                    .setSwapchain(swapchain.handle())
                    .setSemaphore(imageAvailableSemaphore.handle())
                    .setTimeout(UINT64_MAX);
            auto index =
                device.handle()
                    .acquireNextImageKHR(swapchain.handle(), UINT64_MAX,
                                         imageAvailableSemaphore.handle())
                    .value;

            const vk::PipelineStageFlags waitStage =
                vk::PipelineStageFlagBits::eTopOfPipe;

            auto imageAvailableHandle = imageAvailableSemaphore.handle();
            auto renderFinishedHandle = renderFinishedSemaphore.handle();
            const auto submitInfo =
                vk::SubmitInfo()
                    .setCommandBuffers(commandBuffers[index])
                    .setWaitDstStageMask(waitStage)
                    .setWaitSemaphores(imageAvailableHandle)
                    .setSignalSemaphores(renderFinishedHandle);

            device.graphicsQueue().submit({submitInfo}, &fence);

            device.presentQueue().present(swapchain, index,
                                          renderFinishedSemaphore);
        }

        device.handle().waitIdle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
