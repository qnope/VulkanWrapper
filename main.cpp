#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/PhysicalDevice.h>
#include <VulkanWrapper/Vulkan/Queue.h>
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

int main() {
    try {
        vw::SDL_Initializer initializer;
        vw::Window window = vw::WindowBuilder(initializer)
                                .withTitle("Coucou")
                                .sized(800, 600)
                                .build();

        vw::Instance instance =
            vw::InstanceBuilder()
                .addPortability()
                .addExtensions(window.requiredInstanceExtensions())
                .build();

        auto surface = window.createSurface(instance);

        auto device = instance.findGpu()
                          .withQueue(vk::QueueFlagBits::eGraphics |
                                     vk::QueueFlagBits::eCompute |
                                     vk::QueueFlagBits::eTransfer)
                          .withPresentQueue(*surface)
                          .build();

        auto swapchain = window.createSwapchain(device, *surface).build();

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

        const auto framebuffers =
            createFramebuffers(device, renderPass, swapchain);

        while (!window.closeRequested()) {
            window.update();
        }
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
