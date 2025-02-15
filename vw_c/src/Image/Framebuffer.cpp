#include "Image/Framebuffer.h"

#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Image/ImageView.h>

vw::Framebuffer *
vw_create_framebuffer(const VwFramebufferCreateArguments *arguments) {
    auto builder =
        vw::FramebufferBuilder(*arguments->device, *arguments->render_pass,
                               arguments->width, arguments->height);

    for (int i = 0; i < arguments->number_image_views; ++i)
        std::move(builder).add_attachment(*arguments->image_views[i]);

    return new vw::Framebuffer{std::move(builder).build()};
}

uint32_t vw_framebuffer_width(const vw::Framebuffer *framebuffer) {
    return framebuffer->width();
}

uint32_t vw_framebuffer_height(const vw::Framebuffer *framebuffer) {
    return framebuffer->height();
}

VkFramebuffer vw_framebuffer_handle(const vw::Framebuffer *framebuffer) {
    return framebuffer->handle();
}

void vw_destroy_framebuffer(vw::Framebuffer *framebuffer) {
    delete framebuffer;
}
