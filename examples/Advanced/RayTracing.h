#pragma once

#include <vulkan/vulkan.hpp>
#include <VulkanWrapper/Image/ImageView.h>

class RayTracingPass {
  public:
    void execute(vk::CommandBuffer buffer,
                 std::shared_ptr<const vw::ImageView> light_buffer);

  private:
};
