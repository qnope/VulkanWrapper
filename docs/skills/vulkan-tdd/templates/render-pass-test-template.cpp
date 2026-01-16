// Template: Render Pass Test
// Usage: Copy this file for testing render passes with pixel verification

#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <gtest/gtest.h>

namespace {

// ============================================================================
// Shader Sources
// ============================================================================

const std::string FULLSCREEN_VERTEX_SHADER = R"(
#version 450

layout(location = 0) out vec2 fragUV;

void main() {
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0), vec2(1.0, -1.0),
        vec2(-1.0,  1.0), vec2(1.0,  1.0)
    );
    vec2 uvs[4] = vec2[](
        vec2(0.0, 0.0), vec2(1.0, 0.0),
        vec2(0.0, 1.0), vec2(1.0, 1.0)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragUV = uvs[gl_VertexIndex];
}
)";

// Replace with your pass's fragment shader
const std::string YOUR_FRAGMENT_SHADER = R"(
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// Add your uniforms, push constants, samplers here
// layout(push_constant) uniform PushConstants { ... } pc;
// layout(set = 0, binding = 0) uniform sampler2D inputTexture;

void main() {
    // Replace with your shader logic
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

// ============================================================================
// Test Constants
// ============================================================================

constexpr uint32_t TEST_WIDTH = 64;
constexpr uint32_t TEST_HEIGHT = 64;
constexpr size_t BUFFER_SIZE = TEST_WIDTH * TEST_HEIGHT * 4;

// ============================================================================
// Test Fixture
// ============================================================================

class YourRenderPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_device = gpu.device;
        m_allocator = gpu.allocator;
        m_queue = &gpu.queue();

        m_cmdPool = std::make_unique<vw::CommandPool>(
            vw::CommandPoolBuilder(m_device).build());

        // Compile shaders
        vw::ShaderCompiler compiler;
        m_vertexShader = compiler.compile_to_module(
            m_device, FULLSCREEN_VERTEX_SHADER,
            vk::ShaderStageFlagBits::eVertex);
        m_fragmentShader = compiler.compile_to_module(
            m_device, YOUR_FRAGMENT_SHADER,
            vk::ShaderStageFlagBits::eFragment);
    }

    // Helper: Create output image and view
    void create_output_image(uint32_t width, uint32_t height) {
        m_outputImage = m_allocator->create_image_2D(
            vw::Width{width}, vw::Height{height}, false,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eTransferSrc);

        m_outputView = vw::ImageViewBuilder(m_device, m_outputImage)
                           .setImageType(vk::ImageViewType::e2D)
                           .build();
    }

    // Helper: Create staging buffer for readback
    void create_staging_buffer(size_t size) {
        using StagingBuffer =
            vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
        m_stagingBuffer =
            std::make_unique<StagingBuffer>(
                vw::create_buffer<StagingBuffer>(*m_allocator, size));
    }

    // Helper: Read back pixels from image
    std::vector<std::byte> readback_image(uint32_t width, uint32_t height) {
        auto cmd = m_cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        vw::Transfer transfer;
        transfer.copyImageToBuffer(cmd, m_outputImage,
                                   m_stagingBuffer->handle(), 0);

        std::ignore = cmd.end();

        m_queue->enqueue_command_buffer(cmd);
        m_queue->submit({}, {}, {}).wait();

        return m_stagingBuffer->read_as_vector(0, width * height * 4);
    }

    // Helper: Verify pixel color
    void expect_pixel(const std::vector<std::byte> &pixels, size_t index,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                      const std::string &context = "") {
        size_t i = index * 4;
        EXPECT_EQ(static_cast<uint8_t>(pixels[i + 0]), r)
            << "R mismatch " << context;
        EXPECT_EQ(static_cast<uint8_t>(pixels[i + 1]), g)
            << "G mismatch " << context;
        EXPECT_EQ(static_cast<uint8_t>(pixels[i + 2]), b)
            << "B mismatch " << context;
        EXPECT_EQ(static_cast<uint8_t>(pixels[i + 3]), a)
            << "A mismatch " << context;
    }

    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
    vw::Queue *m_queue;
    std::unique_ptr<vw::CommandPool> m_cmdPool;

    std::shared_ptr<vw::ShaderModule> m_vertexShader;
    std::shared_ptr<vw::ShaderModule> m_fragmentShader;

    std::shared_ptr<vw::Image> m_outputImage;
    std::shared_ptr<vw::ImageView> m_outputView;

    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    std::unique_ptr<StagingBuffer> m_stagingBuffer;
};

// ============================================================================
// Tests
// ============================================================================

TEST_F(YourRenderPassTest, RendersCorrectOutput) {
    // Setup
    create_output_image(TEST_WIDTH, TEST_HEIGHT);
    create_staging_buffer(BUFFER_SIZE);

    // Create descriptor layout (modify for your pass)
    auto descriptorLayout =
        vw::DescriptorSetLayoutBuilder(m_device).build();

    // Create pipeline
    auto pipeline = vw::create_screen_space_pipeline(
        m_device, m_vertexShader, m_fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm);

    // Create descriptor set
    auto descriptorPool =
        vw::DescriptorPoolBuilder(m_device, descriptorLayout).build();
    vw::DescriptorAllocator descriptorAllocator;
    auto descriptorSet = descriptorPool.allocate_set(descriptorAllocator);

    // Record rendering
    auto cmd = m_cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    // Transition to color attachment
    tracker.request(vw::Barrier::ImageState{
        .image = m_outputImage->handle(),
        .subresourceRange = m_outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd);

    // Setup color attachment
    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(m_outputView->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    // Begin dynamic rendering
    vk::RenderingInfo renderingInfo =
        vk::RenderingInfo()
            .setRenderArea(vk::Rect2D({0, 0}, {TEST_WIDTH, TEST_HEIGHT}))
            .setLayerCount(1)
            .setColorAttachments(colorAttachment);

    cmd.beginRendering(renderingInfo);

    // Bind pipeline and descriptors
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->handle());
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                           pipeline->layout(), 0, descriptorSet.handle(),
                           nullptr);

    // Set viewport and scissor
    cmd.setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(TEST_WIDTH),
                        static_cast<float>(TEST_HEIGHT), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D({0, 0}, {TEST_WIDTH, TEST_HEIGHT}));

    // Draw fullscreen quad
    cmd.draw(4, 1, 0, 0);

    cmd.endRendering();

    // Copy to staging buffer
    transfer.copyImageToBuffer(cmd, m_outputImage, m_stagingBuffer->handle(),
                               0);

    std::ignore = cmd.end();

    // Submit
    m_queue->enqueue_command_buffer(cmd);
    m_queue->submit({}, {}, {}).wait();

    // Verify output
    auto pixels = m_stagingBuffer->read_as_vector(0, BUFFER_SIZE);

    // Check first pixel (modify expected values for your shader)
    expect_pixel(pixels, 0, 255, 0, 0, 255, "first pixel");

    // Check all pixels are the expected solid color
    for (size_t i = 0; i < TEST_WIDTH * TEST_HEIGHT; ++i) {
        expect_pixel(pixels, i, 255, 0, 0, 255);
    }
}

TEST_F(YourRenderPassTest, HandlesVariousResolutions) {
    struct Resolution {
        uint32_t width;
        uint32_t height;
    };

    std::vector<Resolution> resolutions = {
        {16, 16}, {64, 64}, {128, 128}, {256, 128}, {128, 256}};

    for (const auto &res : resolutions) {
        SCOPED_TRACE("Resolution: " + std::to_string(res.width) + "x" +
                     std::to_string(res.height));

        create_output_image(res.width, res.height);
        create_staging_buffer(res.width * res.height * 4);

        // ... render and verify ...
    }
}

// Add more tests for:
// - Different input parameters
// - Edge cases (zero size, max size)
// - Error conditions
// - Performance characteristics

} // namespace
