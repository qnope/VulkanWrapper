#pragma once

#include "VulkanWrapper/fwd.h"

namespace vw {
void generate_mipmap(vk::CommandBuffer cmd_buffer,
                     const std::shared_ptr<const Image> &image);
}
