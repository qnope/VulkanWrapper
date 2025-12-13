#pragma once

#include <memory>
#include <vector>
#include <VulkanWrapper/fwd.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/ImageView.h>

struct GBuffer {
    std::shared_ptr<const vw::ImageView> color;
    std::shared_ptr<const vw::ImageView> normal;
    std::shared_ptr<const vw::ImageView> tangeant;
    std::shared_ptr<const vw::ImageView> biTangeant;
    std::shared_ptr<const vw::ImageView> position;
    std::shared_ptr<const vw::ImageView> light;
    std::shared_ptr<const vw::ImageView> depth;
    std::shared_ptr<const vw::ImageView> ao;
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
            glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 2.f, 5'000.f);

        proj[1][1] *= -1;
        return proj;
    }();
    // Camera positioned to view curtains and lion head in Sponza
    glm::mat4 view =
        glm::lookAt(glm::vec3(-900.0f, 300.0f, 100.0f), // eye position
                    glm::vec3(500.0f, 100.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)); // up vector

    // Inverse view-projection for reconstructing world position from depth
    glm::mat4 inverseViewProj = glm::inverse(proj * view);
};

// Push constant data for per-object transformation
struct PushConstantData {
    glm::mat4 model = glm::mat4(1.0f);
};
