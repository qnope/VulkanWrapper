#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/RenderPass/ToneMappingPass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>
#include <gtest/gtest.h>

namespace vw::tests {

// =============================================================================
// CPU-side Tonemapping Functions (for verification)
// =============================================================================

namespace {

glm::vec3 tone_map_aces_cpu(glm::vec3 x) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return glm::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

glm::vec3 tone_map_reinhard_cpu(glm::vec3 x) { return x / (1.0f + x); }

glm::vec3 tone_map_reinhard_extended_cpu(glm::vec3 x, float white_point) {
    float w2 = white_point * white_point;
    glm::vec3 numerator = x * (1.0f + x / w2);
    return numerator / (1.0f + x);
}

glm::vec3 uncharted2_partial_cpu(glm::vec3 x) {
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

glm::vec3 tone_map_uncharted2_cpu(glm::vec3 x) {
    const float exposure_bias = 2.0f;
    glm::vec3 curr = uncharted2_partial_cpu(x * exposure_bias);
    const float W = 11.2f;
    glm::vec3 white_scale = 1.0f / uncharted2_partial_cpu(glm::vec3(W));
    return curr * white_scale;
}

std::filesystem::path get_shader_dir() {
    return std::filesystem::path(__FILE__)
               .parent_path()
               .parent_path()
               .parent_path() /
           "Shaders";
}

} // anonymous namespace

// =============================================================================
// Test Fixture
// =============================================================================

class ToneMappingPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();

        cmdPool =
            std::make_unique<CommandPool>(CommandPoolBuilder(device).build());
    }

    std::unique_ptr<ToneMappingPass>
    create_pass(vk::Format format = vk::Format::eR8G8B8A8Unorm) {
        return std::make_unique<ToneMappingPass>(device, allocator,
                                                 get_shader_dir(), format);
    }

    std::shared_ptr<const Image> create_test_image(Width width, Height height,
                                                   vk::Format format,
                                                   vk::ImageUsageFlags usage) {
        return allocator->create_image_2D(width, height, false, format, usage);
    }

    std::shared_ptr<const ImageView>
    create_image_view(std::shared_ptr<const Image> image) {
        return ImageViewBuilder(device, image)
            .setImageType(vk::ImageViewType::e2D)
            .build();
    }

    std::shared_ptr<const ImageView> create_hdr_view(Width width,
                                                     Height height) {
        auto image =
            create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eSampled |
                                  vk::ImageUsageFlagBits::eTransferDst);
        return create_image_view(image);
    }

    std::shared_ptr<const ImageView>
    create_output_view(Width width, Height height, vk::Format format) {
        auto image =
            create_test_image(width, height, format,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eTransferSrc);
        return create_image_view(image);
    }

    void fill_hdr_image(std::shared_ptr<const Image> image, glm::vec4 color) {
        uint32_t width = image->extent2D().width;
        uint32_t height = image->extent2D().height;
        size_t pixel_count = width * height;
        size_t buffer_size = pixel_count * 4 * sizeof(uint16_t);

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto staging = create_buffer<StagingBuffer>(*allocator, buffer_size);

        std::vector<uint16_t> pixels(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            pixels[i * 4 + 0] = glm::packHalf1x16(color.r);
            pixels[i * 4 + 1] = glm::packHalf1x16(color.g);
            pixels[i * 4 + 2] = glm::packHalf1x16(color.b);
            pixels[i * 4 + 3] = glm::packHalf1x16(color.a);
        }
        staging.write(std::span<const std::byte>(
                          reinterpret_cast<const std::byte *>(pixels.data()),
                          buffer_size),
                      0);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyBufferToImage(cmd, staging.handle(), image, 0);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();
    }

    glm::vec4 read_first_pixel(std::shared_ptr<const Image> image) {
        uint32_t width = image->extent2D().width;
        uint32_t height = image->extent2D().height;
        size_t buffer_size = width * height * 4;

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto staging = create_buffer<StagingBuffer>(*allocator, buffer_size);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        auto pixels = staging.read_as_vector(0, buffer_size);

        auto format = image->format();
        if (format == vk::Format::eB8G8R8A8Srgb ||
            format == vk::Format::eB8G8R8A8Unorm) {
            return glm::vec4(static_cast<uint8_t>(pixels[2]) / 255.0f,
                             static_cast<uint8_t>(pixels[1]) / 255.0f,
                             static_cast<uint8_t>(pixels[0]) / 255.0f,
                             static_cast<uint8_t>(pixels[3]) / 255.0f);
        }
        return glm::vec4(static_cast<uint8_t>(pixels[0]) / 255.0f,
                         static_cast<uint8_t>(pixels[1]) / 255.0f,
                         static_cast<uint8_t>(pixels[2]) / 255.0f,
                         static_cast<uint8_t>(pixels[3]) / 255.0f);
    }

    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    Queue *queue;
    std::unique_ptr<CommandPool> cmdPool;
};

// =============================================================================
// Construction & API Tests
// =============================================================================

TEST_F(ToneMappingPassTest, ConstructWithDefaultFormat) {
    auto pass = create_pass();
    ASSERT_NE(pass, nullptr);
}

TEST_F(ToneMappingPassTest, ShaderFilesExistAndCompile) {
    auto shader_dir = get_shader_dir();
    auto vert_path = shader_dir / "fullscreen.vert";
    auto frag_path = shader_dir / "tonemap.frag";

    ASSERT_TRUE(std::filesystem::exists(vert_path))
        << "Vertex shader not found: " << vert_path;
    ASSERT_TRUE(std::filesystem::exists(frag_path))
        << "Fragment shader not found: " << frag_path;

    ShaderCompiler compiler;
    auto vertex_shader = compiler.compile_file_to_module(device, vert_path);
    auto fragment_shader = compiler.compile_file_to_module(device, frag_path);

    ASSERT_NE(vertex_shader, nullptr);
    ASSERT_NE(fragment_shader, nullptr);
}

TEST_F(ToneMappingPassTest, PushConstantsHasCorrectSize) {
    // 4 members: exposure (float), operator_id (int32_t), white_point (float),
    //            luminance_scale (float)
    EXPECT_EQ(sizeof(ToneMappingPass::PushConstants), 16);
}

TEST_F(ToneMappingPassTest, ToneMappingOperatorValues) {
    EXPECT_EQ(static_cast<int32_t>(ToneMappingOperator::ACES), 0);
    EXPECT_EQ(static_cast<int32_t>(ToneMappingOperator::Reinhard), 1);
    EXPECT_EQ(static_cast<int32_t>(ToneMappingOperator::ReinhardExtended), 2);
    EXPECT_EQ(static_cast<int32_t>(ToneMappingOperator::Uncharted2), 3);
    EXPECT_EQ(static_cast<int32_t>(ToneMappingOperator::Neutral), 4);
}

// =============================================================================
// Lazy Allocation Tests
// =============================================================================

TEST_F(ToneMappingPassTest, LazyAllocation_ReturnsValidImageView) {
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();
    auto hdr_view = create_hdr_view(width, height);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result =
        pass->execute(cmd, tracker, Width{width}, Height{height}, 0, hdr_view);

    std::ignore = cmd.end();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->handle());
    EXPECT_EQ(result->image()->extent2D().width, static_cast<uint32_t>(width));
    EXPECT_EQ(result->image()->extent2D().height,
              static_cast<uint32_t>(height));

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();
}

TEST_F(ToneMappingPassTest,
       LazyAllocation_DifferentFrameIndicesCreateDifferentImages) {
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();
    auto hdr_view = create_hdr_view(width, height);

    std::vector<std::shared_ptr<const ImageView>> results;

    for (size_t frame_index = 0; frame_index < 3; ++frame_index) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        auto result = pass->execute(cmd, tracker, Width{width}, Height{height},
                                    frame_index, hdr_view);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        results.push_back(result);
    }

    // Different frame indices should produce different images
    EXPECT_NE(results[0]->image().get(), results[1]->image().get());
    EXPECT_NE(results[1]->image().get(), results[2]->image().get());
}

TEST_F(ToneMappingPassTest, LazyAllocation_SameFrameIndexReusesCachedImage) {
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();
    auto hdr_view = create_hdr_view(width, height);

    // First execution
    auto cmd1 = cmdPool->allocate(1)[0];
    std::ignore = cmd1.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker1;
    auto result1 = pass->execute(cmd1, tracker1, Width{width}, Height{height},
                                 0, hdr_view);

    std::ignore = cmd1.end();
    queue->enqueue_command_buffer(cmd1);
    queue->submit({}, {}, {}).wait();

    // Second execution with same frame index
    auto cmd2 = cmdPool->allocate(1)[0];
    std::ignore = cmd2.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker2;
    auto result2 = pass->execute(cmd2, tracker2, Width{width}, Height{height},
                                 0, hdr_view);

    std::ignore = cmd2.end();
    queue->enqueue_command_buffer(cmd2);
    queue->submit({}, {}, {}).wait();

    EXPECT_EQ(result1->image().get(), result2->image().get());
}

// =============================================================================
// Result Verification Tests
// =============================================================================

TEST_F(ToneMappingPassTest, Verify_NeutralOperatorPassesThrough) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    glm::vec4 input_hdr(0.5f, 0.5f, 0.5f, 1.0f);
    fill_hdr_image(hdr_image, input_hdr);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::Neutral, 1.0f, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);

    constexpr float tolerance = 0.02f;
    EXPECT_NEAR(result.r, 0.5f, tolerance);
    EXPECT_NEAR(result.g, 0.5f, tolerance);
    EXPECT_NEAR(result.b, 0.5f, tolerance);
}

TEST_F(ToneMappingPassTest, Verify_ZeroExposureProducesBlack) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    fill_hdr_image(hdr_image, glm::vec4(10.0f, 10.0f, 10.0f, 1.0f));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::ACES, 0.0f, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);

    constexpr float tolerance = 0.02f;
    EXPECT_NEAR(result.r, 0.0f, tolerance);
    EXPECT_NEAR(result.g, 0.0f, tolerance);
    EXPECT_NEAR(result.b, 0.0f, tolerance);
}

TEST_F(ToneMappingPassTest, Verify_ACESMatchesCPU) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    glm::vec3 hdr_input(2.0f, 2.0f, 2.0f);
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::ACES, 1.0f, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);
    glm::vec3 expected = tone_map_aces_cpu(hdr_input);

    constexpr float tolerance = 0.03f;
    EXPECT_NEAR(result.r, expected.r, tolerance);
    EXPECT_NEAR(result.g, expected.g, tolerance);
    EXPECT_NEAR(result.b, expected.b, tolerance);
}

TEST_F(ToneMappingPassTest, Verify_ReinhardMatchesCPU) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    // Reinhard(1.0) = 1.0 / (1.0 + 1.0) = 0.5
    glm::vec3 hdr_input(1.0f, 1.0f, 1.0f);
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::Reinhard, 1.0f, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);
    glm::vec3 expected = tone_map_reinhard_cpu(hdr_input);

    constexpr float tolerance = 0.03f;
    EXPECT_NEAR(result.r, expected.r, tolerance);
    EXPECT_NEAR(result.g, expected.g, tolerance);
    EXPECT_NEAR(result.b, expected.b, tolerance);
}

TEST_F(ToneMappingPassTest, Verify_Uncharted2MatchesCPU) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    glm::vec3 hdr_input(1.5f, 1.5f, 1.5f);
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::Uncharted2, 1.0f, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);
    glm::vec3 expected = tone_map_uncharted2_cpu(hdr_input);

    constexpr float tolerance = 0.03f;
    EXPECT_NEAR(result.r, expected.r, tolerance);
    EXPECT_NEAR(result.g, expected.g, tolerance);
    EXPECT_NEAR(result.b, expected.b, tolerance);
}

TEST_F(ToneMappingPassTest, Verify_ExposureScalesInput) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    // Input 0.5, exposure 2.0 -> effective input 1.0 -> Reinhard = 0.5
    glm::vec3 hdr_input(0.5f, 0.5f, 0.5f);
    float exposure = 2.0f;
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::Reinhard, exposure, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);
    glm::vec3 expected = tone_map_reinhard_cpu(hdr_input * exposure);

    constexpr float tolerance = 0.03f;
    EXPECT_NEAR(result.r, expected.r, tolerance);
    EXPECT_NEAR(result.g, expected.g, tolerance);
    EXPECT_NEAR(result.b, expected.b, tolerance);
}

TEST_F(ToneMappingPassTest, Verify_ReinhardExtendedWhitePointAffectsResult) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    glm::vec3 hdr_input(3.0f, 3.0f, 3.0f);
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    constexpr float tolerance = 0.03f;

    // Test white point = 4.0
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        // Use luminance_scale=1.0 for backward-compatible behavior
        pass->execute(cmd, tracker, output_view, hdr_view,
                      ToneMappingOperator::ReinhardExtended, 1.0f, 4.0f, 1.0f);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        auto result = read_first_pixel(output_image);
        glm::vec3 expected = tone_map_reinhard_extended_cpu(hdr_input, 4.0f);

        EXPECT_NEAR(result.r, expected.r, tolerance);
        EXPECT_NEAR(result.g, expected.g, tolerance);
        EXPECT_NEAR(result.b, expected.b, tolerance);
    }

    // Test white point = 8.0 (should give different result)
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        // Use luminance_scale=1.0 for backward-compatible behavior
        pass->execute(cmd, tracker, output_view, hdr_view,
                      ToneMappingOperator::ReinhardExtended, 1.0f, 8.0f, 1.0f);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        auto result = read_first_pixel(output_image);
        glm::vec3 expected = tone_map_reinhard_extended_cpu(hdr_input, 8.0f);

        EXPECT_NEAR(result.r, expected.r, tolerance);
        EXPECT_NEAR(result.g, expected.g, tolerance);
        EXPECT_NEAR(result.b, expected.b, tolerance);
    }
}

TEST_F(ToneMappingPassTest, Verify_BrightHDRClipsToWhite) {
    constexpr Width width{4};
    constexpr Height height{4};

    auto pass = create_pass(vk::Format::eR8G8B8A8Unorm);

    auto output_image =
        create_test_image(width, height, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageUsageFlagBits::eColorAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc);
    auto output_view = create_image_view(output_image);

    auto hdr_image =
        create_test_image(width, height, vk::Format::eR16G16B16A16Sfloat,
                          vk::ImageUsageFlagBits::eSampled |
                              vk::ImageUsageFlagBits::eTransferDst);
    auto hdr_view = create_image_view(hdr_image);

    glm::vec3 hdr_input(100.0f, 100.0f, 100.0f);
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    // Use luminance_scale=1.0 for backward-compatible behavior
    pass->execute(cmd, tracker, output_view, hdr_view,
                  ToneMappingOperator::ACES, 1.0f, 4.0f, 1.0f);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto result = read_first_pixel(output_image);

    // For very bright inputs, ACES asymptotes to ~1.0
    EXPECT_GT(result.r, 0.95f) << "Should be near white";
    EXPECT_GT(result.g, 0.95f) << "Should be near white";
    EXPECT_GT(result.b, 0.95f) << "Should be near white";
}

} // namespace vw::tests
