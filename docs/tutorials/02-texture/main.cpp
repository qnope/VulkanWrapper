/**
 * Tutorial 02: Hello Texture
 *
 * This tutorial demonstrates how to load and display a texture using
 * VulkanWrapper. It covers:
 *
 * 1. Loading an image from disk
 * 2. Creating samplers
 * 3. Setting up descriptor sets
 * 4. Sampling textures in shaders
 *
 * Expected output: A textured quad displaying a checkerboard pattern
 */

#include "TutorialFramework/TutorialApp.h"

#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSet.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayoutBuilder.h>
#include <VulkanWrapper/Descriptors/Vertex.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/ImageViewBuilder.h>
#include <VulkanWrapper/Image/SamplerBuilder.h>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Pipeline/GraphicsPipelineBuilder.h>
#include <VulkanWrapper/Pipeline/PipelineLayoutBuilder.h>
#include <VulkanWrapper/Shader/ShaderCompiler.h>

#include <array>

using namespace vw;
using namespace vw::tutorial;

// Vertex with position and texture coordinates
struct TexturedVertex {
    glm::vec3 position;
    glm::vec2 texCoord;

    static constexpr auto bindings() {
        return std::array{vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(TexturedVertex),
            .inputRate = vk::VertexInputRate::eVertex}};
    }

    static constexpr auto attributes() {
        return std::array{
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(TexturedVertex, position)},
            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(TexturedVertex, texCoord)}};
    }
};

// Quad vertices (two triangles)
constexpr std::array<TexturedVertex, 6> quadVertices = {{
    // First triangle
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},  // Top-left
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},   // Top-right
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},    // Bottom-right
    // Second triangle
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},  // Top-left
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},    // Bottom-right
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}    // Bottom-left
}};

// Vertex shader source
constexpr const char *vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
)";

// Fragment shader source
constexpr const char *fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}
)";

class HelloTexture : public TutorialApp {
public:
    HelloTexture()
        : TutorialApp(TutorialConfig{
              .name = "Hello Texture",
              .width = 800,
              .height = 600,
              .frameCount = 1,
              .screenshotPath = "screenshot.png"}) {}

protected:
    void setup() override {
        // Create checkerboard texture
        createCheckerboardTexture();

        // Compile shaders
        ShaderCompiler compiler;

        auto vertexSpirv = compiler.compile(
            vertexShaderSource, shaderc_vertex_shader, "texture.vert");
        auto fragmentSpirv = compiler.compile(
            fragmentShaderSource, shaderc_fragment_shader, "texture.frag");

        m_vertexShader =
            std::make_shared<ShaderModule>(device(), std::move(vertexSpirv));
        m_fragmentShader =
            std::make_shared<ShaderModule>(device(), std::move(fragmentSpirv));

        // Create descriptor set layout
        m_descriptorSetLayout =
            DescriptorSetLayoutBuilder(device())
                .add_binding(0, vk::DescriptorType::eCombinedImageSampler,
                             vk::ShaderStageFlagBits::eFragment)
                .build();

        // Create pipeline layout
        m_pipelineLayout = PipelineLayoutBuilder(device())
                              .add_descriptor_set_layout(m_descriptorSetLayout)
                              .build();

        // Create graphics pipeline
        m_pipeline =
            GraphicsPipelineBuilder(device())
                .set_layout(m_pipelineLayout)
                .add_shader(vk::ShaderStageFlagBits::eVertex, m_vertexShader)
                .add_shader(vk::ShaderStageFlagBits::eFragment, m_fragmentShader)
                .set_vertex_input<TexturedVertex>()
                .set_topology(vk::PrimitiveTopology::eTriangleList)
                .set_polygon_mode(vk::PolygonMode::eFill)
                .set_cull_mode(vk::CullModeFlagBits::eNone)
                .add_dynamic_state(vk::DynamicState::eViewport)
                .add_dynamic_state(vk::DynamicState::eScissor)
                .add_color_attachment(config().colorFormat)
                .build();

        // Create vertex buffer
        m_vertexBuffer = allocator()->create_buffer<TexturedVertex, true,
                                                    VertexBufferUsage>(
            quadVertices.size());
        auto span = m_vertexBuffer->data();
        std::ranges::copy(quadVertices, span.begin());

        // Create descriptor pool and set
        m_descriptorPool = std::make_unique<DescriptorPool>(
            device(), 1, std::vector{vk::DescriptorPoolSize{
                             .type = vk::DescriptorType::eCombinedImageSampler,
                             .descriptorCount = 1}});

        m_descriptorSet = std::make_unique<DescriptorSet>(
            m_descriptorPool->allocate(m_descriptorSetLayout->handle()));

        // Update descriptor set with texture
        m_descriptorSet->update_combined_image_sampler(
            0, m_combinedImage->sampler().handle(),
            m_combinedImage->view().handle(),
            vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    void render(vk::CommandBuffer cmd, uint32_t /*frameIndex*/) override {
        // Transition texture to shader read
        transfer().resourceTracker().request(Barrier::ImageState{
            .image = m_combinedImage->image().handle(),
            .subresourceRange = m_combinedImage->image().full_range(),
            .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .stage = vk::PipelineStageFlagBits2::eFragmentShader,
            .access = vk::AccessFlagBits2::eShaderSampledRead});
        transfer().resourceTracker().flush(cmd);

        // Begin rendering
        beginRendering(cmd, vk::AttachmentLoadOp::eClear,
                       {{{0.1f, 0.1f, 0.15f, 1.0f}}});

        // Bind pipeline and descriptor set
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                         m_pipeline->handle());
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               m_pipelineLayout->handle(), 0,
                               m_descriptorSet->handle(), {});

        // Bind vertex buffer
        cmd.bindVertexBuffers(0, m_vertexBuffer->handle(), vk::DeviceSize{0});

        // Draw quad
        cmd.draw(6, 1, 0, 0);

        // End rendering
        endRendering(cmd);
    }

    void cleanup() override {
        m_descriptorSet.reset();
        m_descriptorPool.reset();
        m_vertexBuffer.reset();
        m_pipeline.reset();
        m_pipelineLayout.reset();
        m_descriptorSetLayout.reset();
        m_fragmentShader.reset();
        m_vertexShader.reset();
        m_combinedImage.reset();
    }

private:
    void createCheckerboardTexture() {
        constexpr uint32_t texWidth = 64;
        constexpr uint32_t texHeight = 64;
        constexpr uint32_t checkerSize = 8;

        // Generate checkerboard pattern
        std::vector<uint8_t> pixels(texWidth * texHeight * 4);
        for (uint32_t y = 0; y < texHeight; ++y) {
            for (uint32_t x = 0; x < texWidth; ++x) {
                uint32_t index = (y * texWidth + x) * 4;
                bool isWhite =
                    ((x / checkerSize) + (y / checkerSize)) % 2 == 0;

                pixels[index + 0] = isWhite ? 255 : 50;   // R
                pixels[index + 1] = isWhite ? 255 : 50;   // G
                pixels[index + 2] = isWhite ? 255 : 50;   // B
                pixels[index + 3] = 255;                   // A
            }
        }

        // Create image
        auto image = allocator()->create_image(
            vk::Format::eR8G8B8A8Unorm, Width{texWidth}, Height{texHeight},
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);

        // Create image view
        auto imageView = ImageViewBuilder(device(), image).as_2d().build();

        // Create sampler
        auto sampler =
            SamplerBuilder(device())
                .set_filter(vk::Filter::eNearest, vk::Filter::eNearest)
                .set_address_mode(vk::SamplerAddressMode::eRepeat)
                .build();

        // Upload texture data
        StagingBufferManager staging(*allocator());
        staging.stage_image(*image, std::span(pixels));

        // Execute upload
        auto commandPool = CommandPoolBuilder(device()).build();
        auto cmds = commandPool.allocate(1);

        // Track image state
        transfer().resourceTracker().track(Barrier::ImageState{
            .image = image->handle(),
            .subresourceRange = image->full_range(),
            .layout = vk::ImageLayout::eUndefined,
            .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
            .access = vk::AccessFlagBits2::eNone});

        transfer().resourceTracker().request(Barrier::ImageState{
            .image = image->handle(),
            .subresourceRange = image->full_range(),
            .layout = vk::ImageLayout::eTransferDstOptimal,
            .stage = vk::PipelineStageFlagBits2::eTransfer,
            .access = vk::AccessFlagBits2::eTransferWrite});

        {
            CommandBufferRecorder recorder(cmds[0]);
            transfer().resourceTracker().flush(cmds[0]);
            staging.flush(cmds[0]);
        }

        vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmds[0]};
        vk::SubmitInfo2 submitInfo{.commandBufferInfoCount = 1,
                                   .pCommandBufferInfos = &cmdInfo};

        Fence fence(device());
        queue().handle().submit2(submitInfo, fence.handle());
        fence.wait();

        m_combinedImage = std::make_unique<CombinedImage>(
            std::move(image), std::move(imageView), std::move(sampler));
    }

    std::unique_ptr<CombinedImage> m_combinedImage;
    std::shared_ptr<ShaderModule> m_vertexShader;
    std::shared_ptr<ShaderModule> m_fragmentShader;
    std::shared_ptr<DescriptorSetLayout> m_descriptorSetLayout;
    std::shared_ptr<PipelineLayout> m_pipelineLayout;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<Buffer<TexturedVertex, true, VertexBufferUsage>>
        m_vertexBuffer;
    std::unique_ptr<DescriptorPool> m_descriptorPool;
    std::unique_ptr<DescriptorSet> m_descriptorSet;
};

int main() {
    try {
        HelloTexture app;
        app.run();
        return 0;
    } catch (const vw::Exception &e) {
        std::cerr << "VulkanWrapper Error: " << e.what() << std::endl;
        std::cerr << "  at " << e.location().file_name() << ":"
                  << e.location().line() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
