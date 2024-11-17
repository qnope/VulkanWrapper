#include <fstream>
#include <VulkanWrapper/Core/Utils/exceptions.h>
#include <VulkanWrapper/Core/Vulkan/Device.h>
#include <VulkanWrapper/Core/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Core/Vulkan/Instance.h>
#include <VulkanWrapper/Core/Vulkan/PhysicalDevice.h>
#include <VulkanWrapper/Core/Vulkan/Queue.h>
#include <VulkanWrapper/Core/Vulkan/Swapchain.h>
#include <VulkanWrapper/Core/Window/SDL_Initializer.h>
#include <VulkanWrapper/Core/Window/Window.h>

std::vector<std::byte> readShader(const std::string &path) {
    constexpr auto flags =
        std::ios_base::ate | std::ios_base::in | std::ios_base::binary;

    std::ifstream in(path, flags);

    if (!in)
        throw 0;

    const std::size_t size = in.tellg();
    std::vector<std::byte> result(size);

    in.seekg(0);

    in.read(reinterpret_cast<char *>(result.data()), size);

    return result;
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

        auto shader1 = readShader("../../Shaders/bin/vert.spv");

        while (!window.closeRequested()) {
            window.update();
        }
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
