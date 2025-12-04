#pragma once

#include <memory>
#include <vector>
#include <VulkanWrapper/fwd.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/ImageView.h>

struct GBuffer {
    std::shared_ptr<const vw::ImageView> color;
    std::shared_ptr<const vw::ImageView> position;
    std::shared_ptr<const vw::ImageView> normal;
    std::shared_ptr<const vw::ImageView> tangeant;
    std::shared_ptr<const vw::ImageView> biTangeant;
    std::shared_ptr<const vw::ImageView> light;
    std::shared_ptr<const vw::ImageView> depth;
};

struct GBufferInformation {
    const GBuffer *gbuffer;
};

struct TonemapInformation {
    vw::CombinedImage color;
    vw::CombinedImage light;
};

struct UBOData {
    glm::mat4 proj = [] {
        auto proj =
            glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 512.f);

        proj[1][1] *= -1;
        return proj;
    }();
    // Camera positioned to see sun (at +X, 30° elevation), cube diagonal view, and plane
    glm::mat4 view =
        glm::lookAt(glm::vec3(-4.0f, 5.0f, 5.0f), glm::vec3(3.0f, 1.5f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));
};

// Push constant data for per-object transformation
struct PushConstantData {
    glm::mat4 model = glm::mat4(1.0f);
};
