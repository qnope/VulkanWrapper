#pragma once

#include <VulkanWrapper/fwd.h>
#include <VulkanWrapper/Image/CombinedImage.h>

struct GBufferInformation {
    const vw::Framebuffer *framebuffer;
};

struct TonemapInformation {
    vw::CombinedImage color;
    vw::CombinedImage light;
};
