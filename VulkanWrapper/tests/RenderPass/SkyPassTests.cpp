#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/RenderPass/SkyPass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <gtest/gtest.h>

namespace vw::tests {

namespace {

std::filesystem::path get_shader_dir() {
    return std::filesystem::path(__FILE__)
               .parent_path()
               .parent_path()
               .parent_path() /
           "Shaders";
}

// Create an inverse view-projection matrix for a camera looking in a given
// direction
glm::mat4 create_inverse_view_proj(glm::vec3 view_direction) {
    // Camera at origin looking in view_direction
    glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Handle case where view direction is parallel to up
    if (std::abs(glm::dot(view_direction, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 view = glm::lookAt(camera_pos, camera_pos + view_direction, up);
    glm::mat4 projection =
        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

    return glm::inverse(projection * view);
}

} // anonymous namespace

// =============================================================================
// Test Fixture
// =============================================================================

class SkyPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();

        cmdPool =
            std::make_unique<CommandPool>(CommandPoolBuilder(device).build());
    }

    std::unique_ptr<SkyPass> create_pass() {
        return std::make_unique<SkyPass>(device, allocator, get_shader_dir());
    }

    std::shared_ptr<const Image> create_depth_image(Width width,
                                                    Height height) {
        return allocator->create_image_2D(
            width, height, false, vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eTransferDst);
    }

    std::shared_ptr<const ImageView>
    create_depth_view(std::shared_ptr<const Image> image) {
        return ImageViewBuilder(device, image)
            .setImageType(vk::ImageViewType::e2D)
            .build();
    }

    // Fill depth buffer with 1.0 (far plane) so sky is rendered everywhere
    void fill_depth_with_far_plane(std::shared_ptr<const Image> depth_image) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        auto &tracker = transfer.resourceTracker();

        // Transition to transfer dst
        tracker.request(
            Barrier::ImageState{.image = depth_image->handle(),
                                .subresourceRange = depth_image->full_range(),
                                .layout = vk::ImageLayout::eTransferDstOptimal,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});
        tracker.flush(cmd);

        // Clear depth to 1.0 (far plane)
        vk::ClearDepthStencilValue clear_value(1.0f, 0);
        vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eDepth, 0, 1,
                                        0, 1);
        cmd.clearDepthStencilImage(depth_image->handle(),
                                   vk::ImageLayout::eTransferDstOptimal,
                                   &clear_value, 1, &range);

        // Transition to depth attachment
        tracker.track(
            Barrier::ImageState{.image = depth_image->handle(),
                                .subresourceRange = depth_image->full_range(),
                                .layout = vk::ImageLayout::eTransferDstOptimal,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});
        tracker.request(Barrier::ImageState{
            .image = depth_image->handle(),
            .subresourceRange = depth_image->full_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead});
        tracker.flush(cmd);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();
    }

    // Read a single pixel from an HDR image (R32G32B32A32Sfloat format)
    glm::vec4 read_pixel_hdr(std::shared_ptr<const Image> image, uint32_t x,
                             uint32_t y) {
        uint32_t width = image->extent2D().width;
        uint32_t height = image->extent2D().height;
        size_t buffer_size = width * height * 4 * sizeof(float);

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
        const float *data = reinterpret_cast<const float *>(pixels.data());

        size_t pixel_idx = (y * width + x) * 4;
        return glm::vec4(data[pixel_idx], data[pixel_idx + 1],
                         data[pixel_idx + 2], data[pixel_idx + 3]);
    }

    // Read the center pixel from an HDR image
    glm::vec4 read_center_pixel_hdr(std::shared_ptr<const Image> image) {
        uint32_t x = image->extent2D().width / 2;
        uint32_t y = image->extent2D().height / 2;
        return read_pixel_hdr(image, x, y);
    }

    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    Queue *queue;
    std::unique_ptr<CommandPool> cmdPool;
};

// =============================================================================
// Construction & API Tests
// =============================================================================

TEST_F(SkyPassTest, ConstructWithDefaultFormats) {
    auto pass = create_pass();
    ASSERT_NE(pass, nullptr);
}

TEST_F(SkyPassTest, ShaderFilesExistAndCompile) {
    auto shader_dir = get_shader_dir();
    auto vert_path = shader_dir / "fullscreen.vert";
    auto frag_path = shader_dir / "sky.frag";

    ASSERT_TRUE(std::filesystem::exists(vert_path))
        << "Vertex shader not found: " << vert_path;
    ASSERT_TRUE(std::filesystem::exists(frag_path))
        << "Fragment shader not found: " << frag_path;

    ShaderCompiler compiler;
    compiler.add_include_path(shader_dir / "include");
    auto vertex_shader = compiler.compile_file_to_module(device, vert_path);
    auto fragment_shader = compiler.compile_file_to_module(device, frag_path);

    ASSERT_NE(vertex_shader, nullptr);
    ASSERT_NE(fragment_shader, nullptr);
}

TEST_F(SkyPassTest, PushConstantsHasCorrectSize) {
    // SkyParametersGPU (96 bytes) + mat4 (64 bytes) = 160 bytes
    EXPECT_EQ(sizeof(SkyPass::PushConstants), 160);
}

TEST_F(SkyPassTest, SkyParametersGPUSize) {
    // 6 vec4s = 96 bytes
    EXPECT_EQ(sizeof(SkyParametersGPU), 96);
}

// =============================================================================
// Lazy Allocation Tests
// =============================================================================

TEST_F(SkyPassTest, LazyAllocation_ReturnsValidImageView) {
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    auto depth_image = create_depth_image(width, height);
    auto depth_view = create_depth_view(depth_image);
    fill_depth_with_far_plane(depth_image);

    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    auto inverse_view_proj = create_inverse_view_proj(glm::vec3(0, 1, 0));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result = pass->execute(cmd, tracker, width, height, 0, depth_view,
                                sky_params, inverse_view_proj);

    std::ignore = cmd.end();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->handle());
    EXPECT_EQ(result->image()->extent2D().width, static_cast<uint32_t>(width));
    EXPECT_EQ(result->image()->extent2D().height,
              static_cast<uint32_t>(height));

    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();
}

TEST_F(SkyPassTest, LazyAllocation_DifferentFrameIndicesCreateDifferentImages) {
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    auto depth_image = create_depth_image(width, height);
    auto depth_view = create_depth_view(depth_image);
    fill_depth_with_far_plane(depth_image);

    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    auto inverse_view_proj = create_inverse_view_proj(glm::vec3(0, 1, 0));

    std::vector<std::shared_ptr<const ImageView>> results;

    for (size_t frame_index = 0; frame_index < 3; ++frame_index) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        auto result = pass->execute(cmd, tracker, width, height, frame_index,
                                    depth_view, sky_params, inverse_view_proj);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        results.push_back(result);
    }

    // Different frame indices should produce different images
    EXPECT_NE(results[0]->image().get(), results[1]->image().get());
    EXPECT_NE(results[1]->image().get(), results[2]->image().get());
}

// =============================================================================
// Sky Rendering Verification Tests
// =============================================================================

TEST_F(SkyPassTest, BlueSkyAtZenith_HighSunProducesBlueColor) {
    // When the sun is high (e.g., 60 degrees above horizon), looking straight
    // up (zenith) should produce predominantly blue sky due to Rayleigh
    // scattering
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    auto depth_image = create_depth_image(width, height);
    auto depth_view = create_depth_view(depth_image);
    fill_depth_with_far_plane(depth_image);

    // Sun high in the sky (60 degrees above horizon)
    auto sky_params = SkyParameters::create_earth_sun(60.0f);

    // Camera looking straight up (zenith direction)
    auto inverse_view_proj = create_inverse_view_proj(glm::vec3(0, 1, 0));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result = pass->execute(cmd, tracker, width, height, 0, depth_view,
                                sky_params, inverse_view_proj);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Read center pixel
    auto color = read_center_pixel_hdr(result->image());

    // Sky should have non-zero luminance
    EXPECT_GT(color.r + color.g + color.b, 0.0f)
        << "Sky should have non-zero luminance";

    // Blue channel should be greater than red channel for blue sky
    // Rayleigh scattering preferentially scatters shorter (blue) wavelengths
    EXPECT_GT(color.b, color.r)
        << "Blue channel should dominate for zenith sky"
        << " (R=" << color.r << ", G=" << color.g << ", B=" << color.b << ")";

    // Blue channel should be greater than or equal to green channel
    EXPECT_GE(color.b, color.g * 0.8f)
        << "Blue should be comparable to or greater than green"
        << " (G=" << color.g << ", B=" << color.b << ")";
}

TEST_F(SkyPassTest, SunsetSky_LowSunProducesWarmColors) {
    // When the sun is near the horizon (sunset), the sky should show warm
    // colors (orange/red) due to increased atmospheric path length
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    auto depth_image = create_depth_image(width, height);
    auto depth_view = create_depth_view(depth_image);
    fill_depth_with_far_plane(depth_image);

    // Sun very low (5 degrees above horizon - sunset)
    auto sky_params = SkyParameters::create_earth_sun(5.0f);

    // Look toward the sun direction (horizon in direction of sun)
    // Sun direction is from sun to planet, so we look in the opposite
    // direction
    glm::vec3 look_toward_sun = -sky_params.star_direction;
    look_toward_sun.y = 0.1f; // Look slightly above horizon
    look_toward_sun = glm::normalize(look_toward_sun);

    auto inverse_view_proj = create_inverse_view_proj(look_toward_sun);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result = pass->execute(cmd, tracker, width, height, 0, depth_view,
                                sky_params, inverse_view_proj);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Read center pixel
    auto color = read_center_pixel_hdr(result->image());

    // Sky should have non-zero luminance
    EXPECT_GT(color.r + color.g + color.b, 0.0f)
        << "Sunset sky should have non-zero luminance";

    // At sunset, blue should be significantly reduced compared to zenith
    // Red and orange colors should be more prominent
    // The ratio of red to blue should be higher than at zenith
    float red_to_blue_ratio =
        (color.b > 0.001f) ? (color.r / color.b) : color.r;

    // For sunset, we expect red to be at least comparable to blue
    // (the exact ratio depends on atmospheric conditions, but red should
    // not be much less than blue)
    EXPECT_GT(red_to_blue_ratio, 0.3f)
        << "At sunset, red should be more prominent relative to blue"
        << " (R=" << color.r << ", B=" << color.b
        << ", ratio=" << red_to_blue_ratio << ")";
}

TEST_F(SkyPassTest, SunDiskVisibility_LookingAtSunShowsBrightDisk) {
    // When looking directly at the sun, the sun disk should be very bright
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    auto depth_image = create_depth_image(width, height);
    auto depth_view = create_depth_view(depth_image);
    fill_depth_with_far_plane(depth_image);

    // Sun at 45 degrees
    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    // Look directly toward the sun
    glm::vec3 look_at_sun = -sky_params.star_direction;
    look_at_sun = glm::normalize(look_at_sun);

    auto inverse_view_proj_at_sun = create_inverse_view_proj(look_at_sun);

    // Also render looking away from sun for comparison
    glm::vec3 look_away = -look_at_sun;
    look_away.y = std::abs(look_away.y); // Keep it above horizon
    look_away = glm::normalize(look_away);
    auto inverse_view_proj_away = create_inverse_view_proj(look_away);

    // Render looking at sun
    glm::vec4 color_at_sun;
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        auto result = pass->execute(cmd, tracker, width, height, 0, depth_view,
                                    sky_params, inverse_view_proj_at_sun);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        color_at_sun = read_center_pixel_hdr(result->image());
    }

    // Render looking away from sun
    glm::vec4 color_away;
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        auto result = pass->execute(cmd, tracker, width, height, 1, depth_view,
                                    sky_params, inverse_view_proj_away);

        std::ignore = cmd.end();
        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        color_away = read_center_pixel_hdr(result->image());
    }

    float luminance_at_sun = color_at_sun.r + color_at_sun.g + color_at_sun.b;
    float luminance_away = color_away.r + color_away.g + color_away.b;

    // Looking at the sun should be significantly brighter than looking away
    // The sun disk adds direct radiance to the scattered sky light
    // Note: Away direction may also include significant sky scattering
    EXPECT_GT(luminance_at_sun, luminance_away * 2.0f)
        << "Sun disk should be at least 2x brighter than sky away from sun"
        << " (at_sun=" << luminance_at_sun << ", away=" << luminance_away
        << ")";

    // The sun should have very high luminance values
    EXPECT_GT(luminance_at_sun, 1000.0f)
        << "Sun disk should have very high luminance (HDR values)"
        << " (luminance=" << luminance_at_sun << ")";
}

TEST_F(SkyPassTest, SkyOutputFormatIsHDR) {
    // Verify the output is in HDR format capable of representing high values
    constexpr Width width{64};
    constexpr Height height{64};

    auto pass = create_pass();

    auto depth_image = create_depth_image(width, height);
    auto depth_view = create_depth_view(depth_image);
    fill_depth_with_far_plane(depth_image);

    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    auto inverse_view_proj = create_inverse_view_proj(glm::vec3(0, 1, 0));

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result = pass->execute(cmd, tracker, width, height, 0, depth_view,
                                sky_params, inverse_view_proj);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // Verify format is HDR (R32G32B32A32Sfloat by default)
    EXPECT_EQ(result->image()->format(), vk::Format::eR32G32B32A32Sfloat);
}

// =============================================================================
// SkyParameters Helper Function Tests
// =============================================================================

TEST_F(SkyPassTest, SkyParameters_AngleToDirection) {
    // 0 degrees = horizon (direction pointing along x-axis)
    auto dir0 = SkyParameters::angle_to_direction(0.0f);
    EXPECT_NEAR(dir0.z, 1.0f, 0.001f);
    EXPECT_NEAR(dir0.y, 0.0f, 0.001f);

    // 90 degrees = zenith (direction pointing up)
    auto dir90 = SkyParameters::angle_to_direction(90.0f);
    EXPECT_NEAR(dir90.z, 0.0f, 0.001f);
    EXPECT_NEAR(dir90.y, 1.0f, 0.001f);

    // 45 degrees = halfway
    auto dir45 = SkyParameters::angle_to_direction(45.0f);
    EXPECT_NEAR(dir45.z, std::cos(glm::radians(45.0f)), 0.001f);
    EXPECT_NEAR(dir45.y, std::sin(glm::radians(45.0f)), 0.001f);
}

TEST_F(SkyPassTest, SkyParameters_TemperatureToColor) {
    // Sun temperature (5778K) should give warm white/yellowish color
    auto sun_color = SkyParameters::temperature_to_color(5778.0f);
    EXPECT_GT(sun_color.r, 0.9f); // High red
    EXPECT_GT(sun_color.g, 0.8f); // High green
    EXPECT_GT(sun_color.b, 0.7f); // Moderate blue

    // Very hot star (10000K) should be bluish
    auto hot_color = SkyParameters::temperature_to_color(10000.0f);
    EXPECT_GT(hot_color.b, hot_color.r * 0.9f); // Blue >= Red

    // Red dwarf (3000K) should be reddish
    auto cool_color = SkyParameters::temperature_to_color(3000.0f);
    EXPECT_GT(cool_color.r, cool_color.b); // Red > Blue
}

TEST_F(SkyPassTest, SkyParameters_AngularDiameterToSolidAngle) {
    // Sun's angular diameter is about 0.53 degrees
    float sun_solid_angle =
        SkyParameters::angular_diameter_to_solid_angle(0.53f);

    // Should be approximately 6.8e-5 steradians
    EXPECT_NEAR(sun_solid_angle, 6.8e-5f, 1e-5f);

    // Larger angular diameter should give larger solid angle
    float larger = SkyParameters::angular_diameter_to_solid_angle(1.0f);
    EXPECT_GT(larger, sun_solid_angle);
}

TEST_F(SkyPassTest, SkyParameters_CreateEarthSun) {
    auto params = SkyParameters::create_earth_sun(45.0f);

    // Check solar constant (1361 W/m^2 for Earth)
    EXPECT_NEAR(params.star_constant, 1361.0f, 1.0f);

    // Check planet radius (6360 km)
    EXPECT_NEAR(params.radius_planet, 6360000.0f, 1000.0f);

    // Check atmosphere radius (6420 km)
    EXPECT_NEAR(params.radius_atmosphere, 6420000.0f, 1000.0f);

    // Check luminous efficiency
    EXPECT_NEAR(params.luminous_efficiency, 93.0f, 1.0f);

    // Star direction should be normalized
    EXPECT_NEAR(glm::length(params.star_direction), 1.0f, 0.001f);

    // Star color should be normalized (components in [0,1])
    EXPECT_GE(params.star_color.r, 0.0f);
    EXPECT_LE(params.star_color.r, 1.0f);
    EXPECT_GE(params.star_color.g, 0.0f);
    EXPECT_LE(params.star_color.g, 1.0f);
    EXPECT_GE(params.star_color.b, 0.0f);
    EXPECT_LE(params.star_color.b, 1.0f);
}

TEST_F(SkyPassTest, SkyParameters_ToGPU) {
    auto params = SkyParameters::create_earth_sun(45.0f);
    auto gpu = params.to_gpu();

    // Verify star direction is packed correctly
    EXPECT_NEAR(gpu.star_direction_and_constant.x, params.star_direction.x,
                0.001f);
    EXPECT_NEAR(gpu.star_direction_and_constant.y, params.star_direction.y,
                0.001f);
    EXPECT_NEAR(gpu.star_direction_and_constant.z, params.star_direction.z,
                0.001f);
    EXPECT_NEAR(gpu.star_direction_and_constant.w, params.star_constant, 0.1f);

    // Verify radii
    EXPECT_NEAR(gpu.radii_and_efficiency.x, params.radius_planet, 1.0f);
    EXPECT_NEAR(gpu.radii_and_efficiency.y, params.radius_atmosphere, 1.0f);
    EXPECT_NEAR(gpu.radii_and_efficiency.z, params.luminous_efficiency, 0.1f);
}

} // namespace vw::tests
