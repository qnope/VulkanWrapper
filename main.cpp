#include <3DRenderer/Core/Utils/exceptions.h>
#include <3DRenderer/Core/Window/SDL_Initializer.h>
#include <3DRenderer/Core/Window/Window.h>

#include <3DRenderer/Core/Vulkan/Device.h>
#include <3DRenderer/Core/Vulkan/DeviceFinder.h>
#include <3DRenderer/Core/Vulkan/Instance.h>
#include <3DRenderer/Core/Vulkan/PhysicalDevice.h>
#include <3DRenderer/Core/Vulkan/Queue.h>

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
                          .withPresentQueue(surface)
                          .build();

        while (!window.closeRequested()) {
            window.update();
        }
    } catch (const r3d::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << std::endl;
    }
}
