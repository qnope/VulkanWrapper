#pragma once

#include <vulkan/vulkan.h>

namespace vw {
class Device;
class ImageView;
class RenderPass;
class Framebuffer;
} // namespace vw

extern "C" {
struct vw_FramebufferArguments {
    const vw::Device *device;
    const vw::RenderPass *render_pass;
    const vw::ImageView **image_views;
    int number_image_views;
    uint32_t width;
    uint32_t height;
};

vw::Framebuffer *vw_create_framebuffer(vw_FramebufferArguments arguments);

void vw_destroy_framebuffer(vw::Framebuffer *framebuffer);
}
