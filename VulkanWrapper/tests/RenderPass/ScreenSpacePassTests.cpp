#include <gtest/gtest.h>
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include "utils/create_gpu.hpp"
#include <cstring>

namespace {

// Fullscreen vertex shader - generates a fullscreen quad
const std::string FULLSCREEN_VERTEX_SHADER = R"(
#version 450

layout(location = 0) out vec2 fragUV;

void main() {
    // Triangle strip: 4 vertices for fullscreen quad
    // Vertex 0: (-1, -1), Vertex 1: (1, -1), Vertex 2: (-1, 1), Vertex 3: (1, 1)
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );

    vec2 uvs[4] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragUV = uvs[gl_VertexIndex];
}
)";

// Simple fragment shader - outputs a solid red color
const std::string SOLID_COLOR_FRAGMENT_SHADER = R"(
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0); // Solid red
}
)";

// Fragment shader with push constants
const std::string PUSH_CONSTANTS_FRAGMENT_SHADER = R"(
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

void main() {
    outColor = pc.color;
}
)";

// Fragment shader that samples a texture
const std::string TEXTURE_SAMPLE_FRAGMENT_SHADER = R"(
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

void main() {
    outColor = texture(inputTexture, fragUV);
}
)";

// UV gradient fragment shader - outputs UV coordinates as color
const std::string UV_GRADIENT_FRAGMENT_SHADER = R"(
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragUV.x, fragUV.y, 0.0, 1.0);
}
)";

// Empty slot enum for passes that don't need image allocation
enum class TestPassSlot {};

// Single slot enum for passes that allocate one image
enum class SingleOutputSlot { Output };

// Test implementation of ScreenSpacePass
template <typename SlotEnum>
class TestScreenSpacePass : public vw::ScreenSpacePass<SlotEnum> {
  public:
    using vw::ScreenSpacePass<SlotEnum>::ScreenSpacePass;

    // Expose protected methods for testing
    std::shared_ptr<const vw::Sampler> test_create_default_sampler() {
        return this->create_default_sampler();
    }

    template <typename PushConstantsType>
    void test_render_fullscreen(vk::CommandBuffer cmd, vk::Extent2D extent,
                                const vk::RenderingAttachmentInfo &color_attachment,
                                const vk::RenderingAttachmentInfo *depth_attachment,
                                const vw::Pipeline &pipeline,
                                const vw::DescriptorSet &descriptor_set,
                                const PushConstantsType &push_constants) {
        this->render_fullscreen(cmd, extent, color_attachment, depth_attachment,
                                pipeline, descriptor_set, push_constants);
    }

    void test_render_fullscreen_no_push(vk::CommandBuffer cmd, vk::Extent2D extent,
                                        const vk::RenderingAttachmentInfo &color_attachment,
                                        const vk::RenderingAttachmentInfo *depth_attachment,
                                        const vw::Pipeline &pipeline,
                                        const vw::DescriptorSet &descriptor_set) {
        this->render_fullscreen(cmd, extent, color_attachment, depth_attachment,
                                pipeline, descriptor_set);
    }

    // Also expose get_or_create_image for testing image allocation
    const vw::CachedImage &test_get_or_create_image(SlotEnum slot, vw::Width width,
                                                     vw::Height height,
                                                     size_t frame_index,
                                                     vk::Format format,
                                                     vk::ImageUsageFlags usage) {
        return this->get_or_create_image(slot, width, height, frame_index, format, usage);
    }
};

} // namespace

class ScreenSpacePassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();

        cmdPool = std::make_unique<vw::CommandPool>(
            vw::CommandPoolBuilder(device).build());
    }

    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
    vw::Queue *queue;
    std::unique_ptr<vw::CommandPool> cmdPool;
};

TEST_F(ScreenSpacePassTest, CreateDefaultSampler) {
    TestScreenSpacePass<TestPassSlot> pass(device, allocator);

    auto sampler = pass.test_create_default_sampler();

    ASSERT_NE(sampler, nullptr);
    EXPECT_TRUE(sampler->handle());
}

// Diagnostic test: verify clear without rendering works
TEST_F(ScreenSpacePassTest, ClearImageDiagnostic) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    // Create output image
    auto outputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst);

    // Create staging buffer for readback
    constexpr size_t bufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, bufferSize);

    // Record and execute
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
                                .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    // Transition to transfer dst for clear
    tracker.request(vw::Barrier::ImageState{
        .image = outputImage->handle(),
        .subresourceRange = outputImage->full_range(),
        .layout = vk::ImageLayout::eTransferDstOptimal,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferWrite});
    tracker.flush(cmd);

    // Clear the image to red
    vk::ClearColorValue clearColor(1.0f, 0.0f, 0.0f, 1.0f);
    vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    cmd.clearColorImage(outputImage->handle(), vk::ImageLayout::eTransferDstOptimal,
                        &clearColor, 1, &range);

    // Copy to staging buffer
    transfer.copyImageToBuffer(cmd, outputImage, stagingBuffer.handle(), 0);

    std::ignore = cmd.end();

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Verify output - should be solid red
    auto pixels = stagingBuffer.as_vector(0, bufferSize);

    EXPECT_EQ(static_cast<uint8_t>(pixels[0]), 255) << "R should be 255";
    EXPECT_EQ(static_cast<uint8_t>(pixels[1]), 0) << "G should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[2]), 0) << "B should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[3]), 255) << "A should be 255";
}

// Diagnostic test: verify dynamic rendering clear (no draw) works
TEST_F(ScreenSpacePassTest, DynamicRenderingClearDiagnostic) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    // Create output image
    auto outputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    auto outputView = vw::ImageViewBuilder(device, outputImage)
                          .setImageType(vk::ImageViewType::e2D)
                          .build();

    // Create staging buffer for readback
    constexpr size_t bufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, bufferSize);

    // Record and execute
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
                                .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    // Transition to color attachment
    tracker.request(vw::Barrier::ImageState{
        .image = outputImage->handle(),
        .subresourceRange = outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd);

    // Setup color attachment with clear to GREEN
    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(outputView->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(0.0f, 1.0f, 0.0f, 1.0f)); // Green

    vk::RenderingInfo renderingInfo =
        vk::RenderingInfo()
            .setRenderArea(vk::Rect2D({0, 0}, {width, height}))
            .setLayerCount(1)
            .setColorAttachments(colorAttachment);

    // Begin and immediately end rendering (just clear)
    cmd.beginRendering(renderingInfo);
    cmd.endRendering();

    // Copy to staging buffer
    transfer.copyImageToBuffer(cmd, outputImage, stagingBuffer.handle(), 0);

    std::ignore = cmd.end();

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Verify output - should be solid green
    auto pixels = stagingBuffer.as_vector(0, bufferSize);

    EXPECT_EQ(static_cast<uint8_t>(pixels[0]), 0) << "R should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[1]), 255) << "G should be 255";
    EXPECT_EQ(static_cast<uint8_t>(pixels[2]), 0) << "B should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[3]), 255) << "A should be 255";
}

TEST_F(ScreenSpacePassTest, RenderSolidColor) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    TestScreenSpacePass<TestPassSlot> pass(device, allocator);

    // Compile shaders
    vw::ShaderCompiler compiler;
    auto vertexShader = compiler.compile_to_module(
        device, FULLSCREEN_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    auto fragmentShader = compiler.compile_to_module(
        device, SOLID_COLOR_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    // Create empty descriptor layout (no descriptors needed)
    auto descriptorLayout = vw::DescriptorSetLayoutBuilder(device).build();

    // Create pipeline
    auto pipeline = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm);

    // Create descriptor pool and allocate empty set
    auto descriptorPool = vw::DescriptorPoolBuilder(device, descriptorLayout).build();
    vw::DescriptorAllocator descriptorAllocator;
    auto descriptorSet = descriptorPool.allocate_set(descriptorAllocator);

    // Create output image
    auto outputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    auto outputView = vw::ImageViewBuilder(device, outputImage)
                          .setImageType(vk::ImageViewType::e2D)
                          .build();

    // Create staging buffer for readback
    constexpr size_t bufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, bufferSize);

    // Record and execute
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
                                .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Use Transfer's resource tracker for proper barrier management
    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    // Transition output image to color attachment
    tracker.request(vw::Barrier::ImageState{
        .image = outputImage->handle(),
        .subresourceRange = outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd);

    // Setup color attachment
    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(outputView->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    // Render fullscreen quad (no push constants version)
    pass.test_render_fullscreen_no_push(cmd, {width, height}, colorAttachment, nullptr,
                                        *pipeline, descriptorSet);

    // Copy to staging buffer (Transfer handles barrier internally)
    transfer.copyImageToBuffer(cmd, outputImage, stagingBuffer.handle(), 0);

    std::ignore = cmd.end();

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Verify output - should be solid red
    auto pixels = stagingBuffer.as_vector(0, bufferSize);

    // Check first pixel is red (R=255, G=0, B=0, A=255)
    EXPECT_EQ(static_cast<uint8_t>(pixels[0]), 255) << "R should be 255";
    EXPECT_EQ(static_cast<uint8_t>(pixels[1]), 0) << "G should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[2]), 0) << "B should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[3]), 255) << "A should be 255";

    // Check all pixels are the same solid red
    for (size_t i = 0; i < width * height; ++i) {
        EXPECT_EQ(static_cast<uint8_t>(pixels[i * 4 + 0]), 255) << "Pixel " << i << " R mismatch";
        EXPECT_EQ(static_cast<uint8_t>(pixels[i * 4 + 1]), 0) << "Pixel " << i << " G mismatch";
        EXPECT_EQ(static_cast<uint8_t>(pixels[i * 4 + 2]), 0) << "Pixel " << i << " B mismatch";
        EXPECT_EQ(static_cast<uint8_t>(pixels[i * 4 + 3]), 255) << "Pixel " << i << " A mismatch";
    }
}

TEST_F(ScreenSpacePassTest, RenderWithPushConstants) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    struct PushConstants {
        float r, g, b, a;
    };

    TestScreenSpacePass<TestPassSlot> pass(device, allocator);

    // Compile shaders
    vw::ShaderCompiler compiler;
    auto vertexShader = compiler.compile_to_module(
        device, FULLSCREEN_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    auto fragmentShader = compiler.compile_to_module(
        device, PUSH_CONSTANTS_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    // Create empty descriptor layout
    auto descriptorLayout = vw::DescriptorSetLayoutBuilder(device).build();

    // Create pipeline with push constants
    std::vector<vk::PushConstantRange> pushConstants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstants))};

    auto pipeline = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm, vk::Format::eUndefined,
        vk::CompareOp::eAlways, pushConstants);

    // Create descriptor pool and allocate empty set
    auto descriptorPool = vw::DescriptorPoolBuilder(device, descriptorLayout).build();
    vw::DescriptorAllocator descriptorAllocator;
    auto descriptorSet = descriptorPool.allocate_set(descriptorAllocator);

    // Create output image
    auto outputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    auto outputView = vw::ImageViewBuilder(device, outputImage)
                          .setImageType(vk::ImageViewType::e2D)
                          .build();

    // Create staging buffer
    constexpr size_t bufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, bufferSize);

    // Record and execute
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
                                .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Use Transfer's resource tracker
    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    tracker.request(vw::Barrier::ImageState{
        .image = outputImage->handle(),
        .subresourceRange = outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd);

    // Setup color attachment
    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(outputView->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    // Push constants for green color
    PushConstants pc{0.0f, 1.0f, 0.0f, 1.0f}; // Green

    // Render fullscreen quad with push constants
    pass.test_render_fullscreen(cmd, {width, height}, colorAttachment, nullptr,
                                *pipeline, descriptorSet, pc);

    // Copy to staging buffer
    transfer.copyImageToBuffer(cmd, outputImage, stagingBuffer.handle(), 0);

    std::ignore = cmd.end();

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Verify output - should be solid green
    auto pixels = stagingBuffer.as_vector(0, bufferSize);

    EXPECT_EQ(static_cast<uint8_t>(pixels[0]), 0) << "R should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[1]), 255) << "G should be 255";
    EXPECT_EQ(static_cast<uint8_t>(pixels[2]), 0) << "B should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[3]), 255) << "A should be 255";
}

TEST_F(ScreenSpacePassTest, RenderUVGradient) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    TestScreenSpacePass<TestPassSlot> pass(device, allocator);

    // Compile shaders
    vw::ShaderCompiler compiler;
    auto vertexShader = compiler.compile_to_module(
        device, FULLSCREEN_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    auto fragmentShader = compiler.compile_to_module(
        device, UV_GRADIENT_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    // Create empty descriptor layout
    auto descriptorLayout = vw::DescriptorSetLayoutBuilder(device).build();

    // Create pipeline
    auto pipeline = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm);

    // Create descriptor pool and allocate empty set
    auto descriptorPool = vw::DescriptorPoolBuilder(device, descriptorLayout).build();
    vw::DescriptorAllocator descriptorAllocator;
    auto descriptorSet = descriptorPool.allocate_set(descriptorAllocator);

    // Create output image
    auto outputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    auto outputView = vw::ImageViewBuilder(device, outputImage)
                          .setImageType(vk::ImageViewType::e2D)
                          .build();

    constexpr size_t bufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto stagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, bufferSize);

    // Record and execute
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
                                .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    tracker.request(vw::Barrier::ImageState{
        .image = outputImage->handle(),
        .subresourceRange = outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd);

    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(outputView->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    pass.test_render_fullscreen_no_push(cmd, {width, height}, colorAttachment, nullptr,
                                        *pipeline, descriptorSet);

    transfer.copyImageToBuffer(cmd, outputImage, stagingBuffer.handle(), 0);

    std::ignore = cmd.end();

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Verify UV gradient
    auto pixels = stagingBuffer.as_vector(0, bufferSize);

    // Check corners - note: pixel centers are at (x+0.5)/width, so corner pixels
    // don't have exactly 0 or 1 UV values. For 64x64:
    // - Pixel (0,0) center: UV ≈ (0.5/64, 0.5/64) ≈ (0.0078, 0.0078) → ~2
    // - Pixel (63,0) center: UV ≈ (63.5/64, 0.5/64) ≈ (0.992, 0.0078) → ~253, ~2
    // Use tolerance of 5 to account for pixel center sampling
    constexpr int tolerance = 5;

    // Top-left (0,0): UV near (0,0) -> R~0, G~0
    EXPECT_NEAR(static_cast<uint8_t>(pixels[0]), 0, tolerance) << "Top-left R";
    EXPECT_NEAR(static_cast<uint8_t>(pixels[1]), 0, tolerance) << "Top-left G";

    // Top-right (63,0): UV near (1,0) -> R~255, G~0
    size_t topRightIdx = (width - 1) * 4;
    EXPECT_NEAR(static_cast<uint8_t>(pixels[topRightIdx + 0]), 255, tolerance) << "Top-right R";
    EXPECT_NEAR(static_cast<uint8_t>(pixels[topRightIdx + 1]), 0, tolerance) << "Top-right G";

    // Bottom-left (0,63): UV near (0,1) -> R~0, G~255
    size_t bottomLeftIdx = (height - 1) * width * 4;
    EXPECT_NEAR(static_cast<uint8_t>(pixels[bottomLeftIdx + 0]), 0, tolerance) << "Bottom-left R";
    EXPECT_NEAR(static_cast<uint8_t>(pixels[bottomLeftIdx + 1]), 255, tolerance) << "Bottom-left G";

    // Bottom-right (63,63): UV near (1,1) -> R~255, G~255
    size_t bottomRightIdx = ((height - 1) * width + (width - 1)) * 4;
    EXPECT_NEAR(static_cast<uint8_t>(pixels[bottomRightIdx + 0]), 255, tolerance) << "Bottom-right R";
    EXPECT_NEAR(static_cast<uint8_t>(pixels[bottomRightIdx + 1]), 255, tolerance) << "Bottom-right G";

    // Verify gradient increases from left to right (R) and top to bottom (G)
    // Check center pixel has intermediate values
    size_t centerIdx = ((height / 2) * width + (width / 2)) * 4;
    EXPECT_NEAR(static_cast<uint8_t>(pixels[centerIdx + 0]), 128, 10) << "Center R";
    EXPECT_NEAR(static_cast<uint8_t>(pixels[centerIdx + 1]), 128, 10) << "Center G";
}

TEST_F(ScreenSpacePassTest, RenderWithTextureInput) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    TestScreenSpacePass<TestPassSlot> pass(device, allocator);

    // Compile shaders
    vw::ShaderCompiler compiler;
    auto vertexShader = compiler.compile_to_module(
        device, FULLSCREEN_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    auto fragmentShader = compiler.compile_to_module(
        device, TEXTURE_SAMPLE_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    // Create descriptor layout with one combined image sampler
    auto descriptorLayout = vw::DescriptorSetLayoutBuilder(device)
                                .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
                                .build();

    // Create pipeline
    auto pipeline = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm);

    // Create descriptor pool
    auto descriptorPool = vw::DescriptorPoolBuilder(device, descriptorLayout).build();

    // Create input texture (blue)
    auto inputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

    auto inputView = vw::ImageViewBuilder(device, inputImage)
                         .setImageType(vk::ImageViewType::e2D)
                         .build();

    // Create sampler
    auto sampler = pass.test_create_default_sampler();

    // Fill input texture with blue color
    constexpr size_t inputBufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto inputStagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, inputBufferSize);

    std::vector<std::byte> bluePixels(inputBufferSize);
    for (size_t i = 0; i < width * height; ++i) {
        bluePixels[i * 4 + 0] = static_cast<std::byte>(0);    // R
        bluePixels[i * 4 + 1] = static_cast<std::byte>(0);    // G
        bluePixels[i * 4 + 2] = static_cast<std::byte>(255);  // B
        bluePixels[i * 4 + 3] = static_cast<std::byte>(255);  // A
    }
    inputStagingBuffer.copy(std::span<const std::byte>(bluePixels), 0);

    // Upload input texture
    auto cmd1 = cmdPool->allocate(1)[0];
    std::ignore = cmd1.begin(vk::CommandBufferBeginInfo()
                                 .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer uploadTransfer;
    uploadTransfer.copyBufferToImage(cmd1, inputStagingBuffer.handle(), inputImage, 0);

    std::ignore = cmd1.end();
    queue->enqueue_command_buffer(cmd1);
    queue->submit({}, {}, {}).wait();

    // Create output image
    auto outputImage = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    auto outputView = vw::ImageViewBuilder(device, outputImage)
                          .setImageType(vk::ImageViewType::e2D)
                          .build();

    auto outputStagingBuffer = vw::create_buffer<StagingBuffer>(*allocator, inputBufferSize);

    // Create descriptor set with input texture
    vw::DescriptorAllocator descriptorAllocator;
    descriptorAllocator.add_combined_image(
        0, vw::CombinedImage(inputView, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);
    auto descriptorSet = descriptorPool.allocate_set(descriptorAllocator);

    // Record rendering
    auto cmd2 = cmdPool->allocate(1)[0];
    std::ignore = cmd2.begin(vk::CommandBufferBeginInfo()
                                 .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    auto &tracker = transfer.resourceTracker();

    // Request barrier states for descriptor resources
    for (const auto &resource : descriptorSet.resources()) {
        tracker.request(resource);
    }

    tracker.request(vw::Barrier::ImageState{
        .image = outputImage->handle(),
        .subresourceRange = outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd2);

    vk::RenderingAttachmentInfo colorAttachment =
        vk::RenderingAttachmentInfo()
            .setImageView(outputView->handle())
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    pass.test_render_fullscreen_no_push(cmd2, {width, height}, colorAttachment, nullptr,
                                        *pipeline, descriptorSet);

    transfer.copyImageToBuffer(cmd2, outputImage, outputStagingBuffer.handle(), 0);

    std::ignore = cmd2.end();
    queue->enqueue_command_buffer(cmd2);
    queue->submit({}, {}, {}).wait();

    // Verify output is blue (same as input)
    auto pixels = outputStagingBuffer.as_vector(0, inputBufferSize);

    EXPECT_EQ(static_cast<uint8_t>(pixels[0]), 0) << "R should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[1]), 0) << "G should be 0";
    EXPECT_EQ(static_cast<uint8_t>(pixels[2]), 255) << "B should be 255";
    EXPECT_EQ(static_cast<uint8_t>(pixels[3]), 255) << "A should be 255";
}

TEST_F(ScreenSpacePassTest, LazyImageAllocation) {
    constexpr uint32_t width = 128;
    constexpr uint32_t height = 128;

    TestScreenSpacePass<SingleOutputSlot> pass(device, allocator);

    // First allocation
    const auto &cached1 = pass.test_get_or_create_image(
        SingleOutputSlot::Output, vw::Width{width}, vw::Height{height}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

    ASSERT_NE(cached1.image, nullptr);
    ASSERT_NE(cached1.view, nullptr);
    EXPECT_EQ(cached1.image->extent2D().width, width);
    EXPECT_EQ(cached1.image->extent2D().height, height);

    // Second request with same params returns cached
    const auto &cached2 = pass.test_get_or_create_image(
        SingleOutputSlot::Output, vw::Width{width}, vw::Height{height}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

    EXPECT_EQ(cached1.image.get(), cached2.image.get());
}

TEST_F(ScreenSpacePassTest, CreateScreenSpacePipelineFunction) {
    vw::ShaderCompiler compiler;
    auto vertexShader = compiler.compile_to_module(
        device, FULLSCREEN_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    auto fragmentShader = compiler.compile_to_module(
        device, SOLID_COLOR_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    auto descriptorLayout = vw::DescriptorSetLayoutBuilder(device).build();

    // Test basic pipeline creation
    auto pipeline1 = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm);

    ASSERT_NE(pipeline1, nullptr);
    EXPECT_TRUE(pipeline1->handle());

    // Test with depth format
    auto pipeline2 = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm, vk::Format::eD32Sfloat, vk::CompareOp::eLess);

    ASSERT_NE(pipeline2, nullptr);
    EXPECT_TRUE(pipeline2->handle());

    // Test with push constants
    std::vector<vk::PushConstantRange> pushConstants = {
        vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, 16)};

    auto pipeline3 = vw::create_screen_space_pipeline(
        device, vertexShader, fragmentShader, descriptorLayout,
        vk::Format::eR8G8B8A8Unorm, vk::Format::eUndefined,
        vk::CompareOp::eAlways, pushConstants);

    ASSERT_NE(pipeline3, nullptr);
    EXPECT_TRUE(pipeline3->handle());
}
