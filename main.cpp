#include <VulkanWrapper/Core/Utils/exceptions.h>
#include <VulkanWrapper/Core/Vulkan/Device.h>
#include <VulkanWrapper/Core/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Core/Vulkan/Instance.h>
#include <VulkanWrapper/Core/Vulkan/PhysicalDevice.h>
#include <VulkanWrapper/Core/Vulkan/Queue.h>
#include <VulkanWrapper/Core/Vulkan/Swapchain.h>
#include <VulkanWrapper/Core/Window/SDL_Initializer.h>
#include <VulkanWrapper/Core/Window/Window.h>

int main() {
    try {
        r3d::SDL_Initializer initializer;
        r3d::Window window = r3d::WindowBuilder(initializer)
                                 .withTitle("Coucou")
                                 .sized(800, 600)
                                 .build();

        r3d::Instance instance =
            r3d::InstanceBuilder()
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

        while (!window.closeRequested()) {
            window.update();
        }
    } catch (const r3d::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
