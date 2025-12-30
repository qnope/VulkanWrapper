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
    glm::mat4 proj = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 inverseViewProj = glm::mat4(1.0f);

    /// Create UBOData with default projection for the given aspect ratio and
    /// view matrix.
    static UBOData create(float aspect_ratio, const glm::mat4 &view_matrix) {
        UBOData data;
        data.proj =
            glm::perspective(glm::radians(60.0f), aspect_ratio, 2.f, 5'000.f);
        data.proj[1][1] *= -1; // Flip Y for Vulkan
        data.view = view_matrix;
        data.inverseViewProj = glm::inverse(data.proj * data.view);
        return data;
    }
};

// Push constant data for per-object transformation
struct PushConstantData {
    glm::mat4 model = glm::mat4(1.0f);
};
