#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/Surface.h"
#include "VulkanWrapper/Vulkan/Swapchain.h"
#include "VulkanWrapper/Window/SDL_Initializer.h"
#include "VulkanWrapper/Window/Window.h"
#include <memory>

class App {
  public:
    App() = default;

  public:
    std::shared_ptr<vw::SDL_Initializer> initializer =
        std::make_shared<vw::SDL_Initializer>();
    vw::Window window = vw::WindowBuilder(initializer)
                            .with_title("Vulkan Wrapper")
                            .sized(vw::Width(1600), vw::Height(900))
                            .build();

    std::shared_ptr<vw::Instance> instance =
        vw::InstanceBuilder()
            .addPortability()
            .addExtensions(window.get_required_instance_extensions())
            .setApiVersion(vw::ApiVersion::e13)
            .build();

    vw::Surface surface = window.create_surface(*instance);

    std::shared_ptr<vw::Device> device =
        instance->findGpu()
            .with_queue(vk::QueueFlagBits::eGraphics |
                        vk::QueueFlagBits::eCompute |
                        vk::QueueFlagBits::eTransfer)
            .with_presentation(surface.handle())
            .with_synchronization_2()
            .with_ray_tracing()
            .with_dynamic_rendering()
            .build();

    std::shared_ptr<vw::Allocator> allocator =
        vw::AllocatorBuilder(instance, device).build();

    vw::Swapchain swapchain = window.create_swapchain(device, surface.handle());
};
