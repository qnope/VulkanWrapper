#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/PhysicalDevice.h>
#include <VulkanWrapper/Vulkan/Queue.h>
#include <VulkanWrapper/Vulkan/Swapchain.h>
#include <VulkanWrapper/Window/SDL_Initializer.h>
#include <VulkanWrapper/Window/Window.h>

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

        auto pipeline =
            vw::GraphicsPipelineBuilder()
                .addShaderModule(vk::ShaderStageFlagBits::eVertex,
                                 std::move(vertexShader))
                .addShaderModule(vk::ShaderStageFlagBits::eFragment,
                                 std::move(fragmentShader))
                .withFixedViewport(swapchain.width(), swapchain.height())
                .withFixedScissor(swapchain.width(), swapchain.height())
                .withPipelineLayout(pipelineLayout)
                .addColorAttachment()
                .build(device);

        while (!window.closeRequested()) {
            window.update();
        }
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
