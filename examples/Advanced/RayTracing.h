#pragma once

#include <vulkan/vulkan.hpp>
#include <VulkanWrapper/fwd.h>

class RayTracingPass {
  public:
    void execute(vk::CommandBuffer command_buffer,
                 const vw::Framebuffer &framebuffer);

  private:
};
