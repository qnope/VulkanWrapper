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

struct UBOData {
    glm::mat4 proj = [] {
        auto proj = glm::perspective(glm::radians(60.0F), 1600.0F / 900.0F, 1.F,
                                     10000.0F);
        proj[1][1] *= -1;
        return proj;
    }();
    glm::mat4 view =
        glm::lookAt(glm::vec3(3.0F, 4.0F, 2.0F), glm::vec3(.0F, 0.F, 0.0F),
                    glm::vec3(0.0F, 1.0F, 0.0F));
    glm::mat4 model = glm::mat4(1.0);
};
