#pragma once

#include "vulkan/vulkan.h"

extern "C" {
enum class VwQueueFlagBits {
    Graphics = VK_QUEUE_GRAPHICS_BIT,
    Compute = VK_QUEUE_COMPUTE_BIT,
    Transfer = VK_QUEUE_TRANSFER_BIT
};

enum class VwImageLayout {
    Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
    AttachmentOptimal = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    PresentSrcKHR = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

enum class VwFormat {
    Undefined = VK_FORMAT_UNDEFINED,
    B8G8R8A8_SRGB = VK_FORMAT_B8G8R8A8_SRGB
};

enum class VwShaderStageFlagBits {
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    Compute = VK_SHADER_STAGE_COMPUTE_BIT
};

enum class VwImageViewType { Type2D = VK_IMAGE_VIEW_TYPE_2D };

enum class VwPipelineStageFlagBits {
    TopOfPipe = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
};
}
