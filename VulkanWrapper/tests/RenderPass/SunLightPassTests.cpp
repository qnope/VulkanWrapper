#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Model/MeshManager.h"
#include "VulkanWrapper/RayTracing/RayTracedScene.h"
#include "VulkanWrapper/RenderPass/SunLightPass.h"
#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/DeviceFinder.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <gtest/gtest.h>
#include <optional>

namespace vw::tests {

namespace {

std::filesystem::path get_shader_dir() {
    return std::filesystem::path(__FILE__)
               .parent_path()
               .parent_path()
               .parent_path() /
           "Shaders";
}

// Ray tracing GPU with mesh loading capabilities
struct RayTracingGPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    std::optional<Model::MeshManager> mesh_manager;

    Queue &queue() { return device->graphicsQueue(); }

    void ensure_meshes_loaded() {
        if (!mesh_manager) {
            mesh_manager.emplace(device, allocator);
            // Use relative path from build/VulkanWrapper/tests/ to Models/
            mesh_manager->read_file("../../../Models/cube.obj");
            mesh_manager->read_file("../../../Models/plane.obj");
            auto cmd = mesh_manager->fill_command_buffer();
            queue().enqueue_command_buffer(cmd);
            queue().submit({}, {}, {}).wait();
        }
    }

    const Model::Mesh &get_cube_mesh() {
        ensure_meshes_loaded();
        return mesh_manager->meshes()[0];
    }

    const Model::Mesh &get_plane_mesh() {
        ensure_meshes_loaded();
        return mesh_manager->meshes()[1];
    }
};

RayTracingGPU *create_ray_tracing_gpu() {
    try {
        auto instance =
            InstanceBuilder().setDebug().setApiVersion(ApiVersion::e13).build();

        auto device = instance->findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics)
                          .with_synchronization_2()
                          .with_dynamic_rendering()
                          .with_ray_tracing()
                          .with_descriptor_indexing()
                          .build();

        auto allocator = AllocatorBuilder(instance, device).build();

        // Intentionally leak to avoid static destruction order issues
        return new RayTracingGPU{std::move(instance), std::move(device),
                                 std::move(allocator), std::nullopt};
    } catch (...) {
        return nullptr;
    }
}

RayTracingGPU *get_ray_tracing_gpu() {
    static RayTracingGPU *gpu = create_ray_tracing_gpu();
    return gpu;
}

} // anonymous namespace

// =============================================================================
// Test Fixture
// =============================================================================

class SunLightPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP() << "Ray tracing not available on this system";
        }

        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(gpu->device).build());
    }

    // Create G-buffer images for testing
    struct GBuffer {
        std::shared_ptr<const Image> color;
        std::shared_ptr<const ImageView> color_view;
        std::shared_ptr<const Image> position;
        std::shared_ptr<const ImageView> position_view;
        std::shared_ptr<const Image> normal;
        std::shared_ptr<const ImageView> normal_view;
        std::shared_ptr<const Image> ao;
        std::shared_ptr<const ImageView> ao_view;
        std::shared_ptr<const Image> depth;
        std::shared_ptr<const ImageView> depth_view;
        std::shared_ptr<const Image> light;
        std::shared_ptr<const ImageView> light_view;
    };

    GBuffer create_gbuffer(Width width, Height height) {
        GBuffer gb;

        // Color (albedo)
        gb.color = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.color_view = ImageViewBuilder(gpu->device, gb.color)
                            .setImageType(vk::ImageViewType::e2D)
                            .build();

        // World position
        gb.position = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.position_view = ImageViewBuilder(gpu->device, gb.position)
                               .setImageType(vk::ImageViewType::e2D)
                               .build();

        // World normal
        gb.normal = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.normal_view = ImageViewBuilder(gpu->device, gb.normal)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();

        // Ambient occlusion
        gb.ao = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.ao_view = ImageViewBuilder(gpu->device, gb.ao)
                         .setImageType(vk::ImageViewType::e2D)
                         .build();

        // Depth
        gb.depth = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.depth_view = ImageViewBuilder(gpu->device, gb.depth)
                            .setImageType(vk::ImageViewType::e2D)
                            .build();

        // Light output (HDR)
        gb.light = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.light_view = ImageViewBuilder(gpu->device, gb.light)
                            .setImageType(vk::ImageViewType::e2D)
                            .build();

        return gb;
    }

    // Fill G-buffer with test data for a surface facing a given direction
    void fill_gbuffer_uniform(GBuffer &gb, glm::vec3 albedo, glm::vec3 position,
                              glm::vec3 normal, float ao, float depth_value) {
        uint32_t width = gb.color->extent2D().width;
        uint32_t height = gb.color->extent2D().height;
        size_t pixel_count = width * height;

        // Create staging buffers
        size_t float4_size = pixel_count * 4 * sizeof(float);
        size_t float1_size = pixel_count * sizeof(float);

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto color_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto position_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto normal_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto ao_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float1_size);

        // Fill color
        std::vector<float> color_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            color_data[i * 4 + 0] = albedo.r;
            color_data[i * 4 + 1] = albedo.g;
            color_data[i * 4 + 2] = albedo.b;
            color_data[i * 4 + 3] = 1.0f;
        }
        color_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(color_data.data()),
                float4_size),
            0);

        // Fill position
        std::vector<float> position_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            position_data[i * 4 + 0] = position.x;
            position_data[i * 4 + 1] = position.y;
            position_data[i * 4 + 2] = position.z;
            position_data[i * 4 + 3] = 1.0f;
        }
        position_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(position_data.data()),
                float4_size),
            0);

        // Fill normal
        std::vector<float> normal_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            normal_data[i * 4 + 0] = normal.x;
            normal_data[i * 4 + 1] = normal.y;
            normal_data[i * 4 + 2] = normal.z;
            normal_data[i * 4 + 3] = 0.0f;
        }
        normal_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(normal_data.data()),
                float4_size),
            0);

        // Fill AO
        std::vector<float> ao_data(pixel_count, ao);
        ao_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(ao_data.data()),
                float1_size),
            0);

        // Upload all data
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyBufferToImage(cmd, color_staging.handle(), gb.color, 0);
        transfer.copyBufferToImage(cmd, position_staging.handle(), gb.position,
                                   0);
        transfer.copyBufferToImage(cmd, normal_staging.handle(), gb.normal, 0);
        transfer.copyBufferToImage(cmd, ao_staging.handle(), gb.ao, 0);

        // Clear depth
        auto &tracker = transfer.resourceTracker();
        tracker.request(
            Barrier::ImageState{.image = gb.depth->handle(),
                                .subresourceRange = gb.depth->full_range(),
                                .layout = vk::ImageLayout::eTransferDstOptimal,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});
        tracker.flush(cmd);

        vk::ClearDepthStencilValue clear_depth(depth_value, 0);
        vk::ImageSubresourceRange depth_range(vk::ImageAspectFlagBits::eDepth,
                                              0, 1, 0, 1);
        cmd.clearDepthStencilImage(gb.depth->handle(),
                                   vk::ImageLayout::eTransferDstOptimal,
                                   &clear_depth, 1, &depth_range);

        // Transition depth to attachment
        tracker.track(
            Barrier::ImageState{.image = gb.depth->handle(),
                                .subresourceRange = gb.depth->full_range(),
                                .layout = vk::ImageLayout::eTransferDstOptimal,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});
        tracker.request(Barrier::ImageState{
            .image = gb.depth->handle(),
            .subresourceRange = gb.depth->full_range(),
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead});

        // Clear light buffer to zero
        tracker.request(
            Barrier::ImageState{.image = gb.light->handle(),
                                .subresourceRange = gb.light->full_range(),
                                .layout = vk::ImageLayout::eTransferDstOptimal,
                                .stage = vk::PipelineStageFlagBits2::eTransfer,
                                .access = vk::AccessFlagBits2::eTransferWrite});
        tracker.flush(cmd);

        vk::ClearColorValue clear_light(0.0f, 0.0f, 0.0f, 0.0f);
        vk::ImageSubresourceRange light_range(vk::ImageAspectFlagBits::eColor,
                                              0, 1, 0, 1);
        cmd.clearColorImage(gb.light->handle(),
                            vk::ImageLayout::eTransferDstOptimal, &clear_light,
                            1, &light_range);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    // Read center pixel from light buffer (R16G16B16A16Sfloat)
    glm::vec4 read_center_pixel_light(std::shared_ptr<const Image> image) {
        uint32_t width = image->extent2D().width;
        uint32_t height = image->extent2D().height;
        size_t buffer_size = width * height * 4 * sizeof(uint16_t);

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto staging =
            create_buffer<StagingBuffer>(*gpu->allocator, buffer_size);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        auto pixels = staging.read_as_vector(0, buffer_size);
        const uint16_t *data =
            reinterpret_cast<const uint16_t *>(pixels.data());

        size_t x = width / 2;
        size_t y = height / 2;
        size_t pixel_idx = (y * width + x) * 4;

        return glm::vec4(glm::unpackHalf1x16(data[pixel_idx]),
                         glm::unpackHalf1x16(data[pixel_idx + 1]),
                         glm::unpackHalf1x16(data[pixel_idx + 2]),
                         glm::unpackHalf1x16(data[pixel_idx + 3]));
    }

    RayTracingGPU *gpu = nullptr;
    std::unique_ptr<CommandPool> cmdPool;
};

// =============================================================================
// Construction & API Tests
// =============================================================================

TEST_F(SunLightPassTest, ShaderFilesExistAndCompile) {
    auto shader_dir = get_shader_dir();
    auto vert_path = shader_dir / "fullscreen.vert";
    auto frag_path = shader_dir / "sun_light.frag";

    ASSERT_TRUE(std::filesystem::exists(vert_path))
        << "Vertex shader not found: " << vert_path;
    ASSERT_TRUE(std::filesystem::exists(frag_path))
        << "Fragment shader not found: " << frag_path;

    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto vertex_shader =
        compiler.compile_file_to_module(gpu->device, vert_path);
    auto fragment_shader =
        compiler.compile_file_to_module(gpu->device, frag_path);

    ASSERT_NE(vertex_shader, nullptr);
    ASSERT_NE(fragment_shader, nullptr);
}

TEST_F(SunLightPassTest, PushConstantsMatchesSkyParametersGPU) {
    EXPECT_EQ(sizeof(SunLightPass::PushConstants), sizeof(SkyParametersGPU));
    EXPECT_EQ(sizeof(SunLightPass::PushConstants), 96);
}

// =============================================================================
// Diffuse Lighting Tests
// =============================================================================

TEST_F(SunLightPassTest, BasicDiffuseLighting_SurfaceFacingSunReceivesLight) {
    // A surface with normal pointing toward the sun should receive light
    constexpr Width width{64};
    constexpr Height height{64};

    // Create scene with no geometry (no shadows)
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    // Place plane far below so it does not block light
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<SunLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

    auto gb = create_gbuffer(width, height);

    // Sun at 45 degrees above horizon
    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    // Surface at origin with normal pointing toward the sun
    glm::vec3 normal_toward_sun = glm::normalize(-sky_params.star_direction);
    fill_gbuffer_uniform(gb, glm::vec3(1.0f), // white albedo
                         glm::vec3(0, 0, 0),  // position at origin
                         normal_toward_sun,   // facing sun
                         1.0f,                // full AO
                         0.5f);               // some depth

    // Execute pass
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                  gb.position_view, gb.normal_view, sky_params);

    std::ignore = cmd.end();
    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();

    auto color = read_center_pixel_light(gb.light);

    // Surface facing sun should receive significant light
    float luminance = color.r + color.g + color.b;
    EXPECT_GT(luminance, 0.0f)
        << "Surface facing sun should receive light (R=" << color.r
        << ", G=" << color.g << ", B=" << color.b << ")";
}

TEST_F(SunLightPassTest, BasicDiffuseLighting_SurfaceFacingAwayIsDark) {
    // A surface with normal pointing away from the sun should receive
    // only ambient light
    constexpr Width width{64};
    constexpr Height height{64};

    // Create scene with no geometry (no shadows)
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<SunLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

    auto gb = create_gbuffer(width, height);

    // Sun at 45 degrees
    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    // Surface with normal pointing away from sun (opposite direction)
    glm::vec3 normal_away = glm::normalize(sky_params.star_direction);
    fill_gbuffer_uniform(gb, glm::vec3(1.0f), // white albedo
                         glm::vec3(0, 0, 0),  // position at origin
                         normal_away,         // facing away from sun
                         1.0f,                // full AO
                         0.5f);               // some depth

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                  gb.position_view, gb.normal_view, sky_params);

    std::ignore = cmd.end();
    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();

    auto color = read_center_pixel_light(gb.light);

    // Surface facing away should receive no direct light
    // (ambient/indirect is now handled by SkyLightPass, not SunLightPass)
    float luminance = color.r + color.g + color.b;

    // Should have zero direct light since normal faces away from sun
    EXPECT_EQ(luminance, 0.0f)
        << "Surface facing away from sun should receive no direct light";
}

TEST_F(SunLightPassTest, DiffuseLighting_FacingSunVsFacingAway) {
    // Compare light received by surfaces facing vs facing away from sun
    constexpr Width width{64};
    constexpr Height height{64};

    // Create scene with no geometry
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<SunLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

    // Render surface facing sun
    glm::vec4 color_facing_sun;
    {
        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0, 0, 0), to_sun,
                             1.0f, 0.5f);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                      gb.position_view, gb.normal_view, sky_params);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        color_facing_sun = read_center_pixel_light(gb.light);
    }

    // Render surface facing away
    glm::vec4 color_facing_away;
    {
        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0, 0, 0), -to_sun,
                             1.0f, 0.5f);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                      gb.position_view, gb.normal_view, sky_params);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        color_facing_away = read_center_pixel_light(gb.light);
    }

    float luminance_facing =
        color_facing_sun.r + color_facing_sun.g + color_facing_sun.b;
    float luminance_away =
        color_facing_away.r + color_facing_away.g + color_facing_away.b;

    // Surface facing sun should receive significantly more light
    EXPECT_GT(luminance_facing, luminance_away * 2.0f)
        << "Surface facing sun should receive at least 2x more light"
        << " (facing=" << luminance_facing << ", away=" << luminance_away
        << ")";
}

// =============================================================================
// Shadow Tests
// =============================================================================

TEST_F(SunLightPassTest, ShadowOcclusion_BlockedSurfaceReceivesOnlyAmbient) {
    // A surface in shadow (blocked by geometry) should receive only ambient
    constexpr Width width{64};
    constexpr Height height{64};

    // Create scene with a large occluder plane between the surface and sun
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();

    // Sun is at 45 degrees, star_direction points FROM sun
    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

    // Position occluder plane between origin and sun
    // The plane is oriented facing downward (blocking light from above)
    glm::mat4 occluder_transform = glm::mat4(1.0f);
    occluder_transform = glm::translate(occluder_transform,
                                        to_sun * 50.0f); // 50 units toward sun
    occluder_transform =
        glm::scale(occluder_transform, glm::vec3(100.0f)); // Large plane
    std::ignore = scene.add_instance(plane, occluder_transform);

    scene.build();

    auto pass = std::make_unique<SunLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

    auto gb = create_gbuffer(width, height);

    // Surface at origin facing the sun (would be lit if not shadowed)
    fill_gbuffer_uniform(gb, glm::vec3(1.0f), // white albedo
                         glm::vec3(0, 0, 0),  // position at origin
                         to_sun,              // facing sun
                         1.0f,                // full AO
                         0.5f);               // some depth

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                  gb.position_view, gb.normal_view, sky_params);

    std::ignore = cmd.end();
    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();

    auto color_shadowed = read_center_pixel_light(gb.light);
    float luminance_shadowed =
        color_shadowed.r + color_shadowed.g + color_shadowed.b;

    // Shadowed surface should receive no direct light
    // (ambient/indirect is now handled by SkyLightPass, not SunLightPass)
    EXPECT_EQ(luminance_shadowed, 0.0f)
        << "Shadowed surface should receive no direct light";
}

TEST_F(SunLightPassTest, ShadowOcclusion_LitVsShadowed) {
    // Compare lit surface vs shadowed surface
    constexpr Width width{64};
    constexpr Height height{64};

    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

    // First: render unshadowed
    glm::vec4 color_lit;
    {
        rt::RayTracedScene scene(gpu->device, gpu->allocator);
        const auto &plane = gpu->get_plane_mesh();
        // Occluder far away, not blocking
        std::ignore = scene.add_instance(
            plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
        scene.build();

        auto pass = std::make_unique<SunLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0, 0, 0), to_sun,
                             1.0f, 0.5f);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                      gb.position_view, gb.normal_view, sky_params);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        color_lit = read_center_pixel_light(gb.light);
    }

    // Second: render shadowed
    glm::vec4 color_shadowed;
    {
        rt::RayTracedScene scene(gpu->device, gpu->allocator);
        const auto &plane = gpu->get_plane_mesh();
        // Large occluder between surface and sun
        glm::mat4 occluder = glm::translate(glm::mat4(1.0f), to_sun * 50.0f);
        occluder = glm::scale(occluder, glm::vec3(100.0f));
        std::ignore = scene.add_instance(plane, occluder);
        scene.build();

        auto pass = std::make_unique<SunLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0, 0, 0), to_sun,
                             1.0f, 0.5f);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                      gb.position_view, gb.normal_view, sky_params);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        color_shadowed = read_center_pixel_light(gb.light);
    }

    float luminance_lit = color_lit.r + color_lit.g + color_lit.b;
    float luminance_shadowed =
        color_shadowed.r + color_shadowed.g + color_shadowed.b;

    // Lit surface should receive significantly more light than shadowed
    EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)
        << "Lit surface should receive at least 2x more light than shadowed"
        << " (lit=" << luminance_lit << ", shadowed=" << luminance_shadowed
        << ")";
}

// =============================================================================
// Atmospheric Attenuation Tests
// =============================================================================

TEST_F(SunLightPassTest, AtmosphericAttenuation_SunsetIsWarmerThanNoon) {
    // Light at sunset should be warmer (more red, less blue) than at noon
    // due to increased atmospheric scattering path
    constexpr Width width{64};
    constexpr Height height{64};

    // Scene with no shadows
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<SunLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas_handle());

    // Noon (sun high, 70 degrees)
    glm::vec4 color_noon;
    {
        auto sky_params = SkyParameters::create_earth_sun(70.0f);
        glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0, 0, 0), to_sun,
                             1.0f, 0.5f);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                      gb.position_view, gb.normal_view, sky_params);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        color_noon = read_center_pixel_light(gb.light);
    }

    // Sunset (sun low, 5 degrees)
    glm::vec4 color_sunset;
    {
        auto sky_params = SkyParameters::create_earth_sun(5.0f);
        glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0, 0, 0), to_sun,
                             1.0f, 0.5f);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Barrier::ResourceTracker tracker;
        pass->execute(cmd, tracker, gb.light_view, gb.depth_view, gb.color_view,
                      gb.position_view, gb.normal_view, sky_params);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        color_sunset = read_center_pixel_light(gb.light);
    }

    // Calculate color temperature indicators
    // At noon: more balanced (whiter light)
    // At sunset: warmer (red shifted, blue reduced)

    // Sunset should have higher red-to-blue ratio than noon
    float noon_rb_ratio =
        (color_noon.b > 0.001f) ? (color_noon.r / color_noon.b) : color_noon.r;
    float sunset_rb_ratio = (color_sunset.b > 0.001f)
                                ? (color_sunset.r / color_sunset.b)
                                : color_sunset.r;

    EXPECT_GT(sunset_rb_ratio, noon_rb_ratio)
        << "Sunset should have higher red-to-blue ratio than noon"
        << " (noon R/B=" << noon_rb_ratio << ", sunset R/B=" << sunset_rb_ratio
        << ")";

    // Additionally, sunset should have less total light (more attenuation)
    float luminance_noon = color_noon.r + color_noon.g + color_noon.b;
    float luminance_sunset = color_sunset.r + color_sunset.g + color_sunset.b;

    EXPECT_GT(luminance_noon, luminance_sunset)
        << "Noon should have higher total luminance than sunset"
        << " (noon=" << luminance_noon << ", sunset=" << luminance_sunset
        << ")";
}

} // namespace vw::tests
