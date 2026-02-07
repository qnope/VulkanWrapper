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
#include "VulkanWrapper/RenderPass/IndirectLightPass.h"
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
            mesh_manager->read_file("../../../Models/plane.obj");
            mesh_manager->read_file("../../../Models/cube.obj");
            auto cmd = mesh_manager->fill_command_buffer();
            queue().enqueue_command_buffer(cmd);
            queue().submit({}, {}, {}).wait();
        }
    }

    const Model::Mesh &get_plane_mesh() {
        ensure_meshes_loaded();
        return mesh_manager->meshes()[0];
    }

    const Model::Mesh &get_cube_mesh() {
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
// Test Fixture for Sun Bounce
// =============================================================================

class IndirectLightSunBounceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP() << "Ray tracing not available on this system";
        }

        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(gpu->device).build());
    }

    // Simplified G-buffer for indirect light
    struct GBuffer {
        std::shared_ptr<const Image> position;
        std::shared_ptr<const ImageView> position_view;
        std::shared_ptr<const Image> normal;
        std::shared_ptr<const ImageView> normal_view;
        std::shared_ptr<const Image> albedo;
        std::shared_ptr<const ImageView> albedo_view;
        std::shared_ptr<const Image> ao;
        std::shared_ptr<const ImageView> ao_view;
        std::shared_ptr<const Image> tangent;
        std::shared_ptr<const ImageView> tangent_view;
        std::shared_ptr<const Image> bitangent;
        std::shared_ptr<const ImageView> bitangent_view;
    };

    GBuffer create_gbuffer(Width width, Height height) {
        GBuffer gb;
        auto fmt = vk::Format::eR32G32B32A32Sfloat;
        auto usage = vk::ImageUsageFlagBits::eSampled |
                     vk::ImageUsageFlagBits::eTransferDst;

        gb.position = gpu->allocator->create_image_2D(width, height, false,
                                                       fmt, usage);
        gb.position_view = ImageViewBuilder(gpu->device, gb.position)
                               .setImageType(vk::ImageViewType::e2D)
                               .build();

        gb.normal = gpu->allocator->create_image_2D(width, height, false,
                                                     fmt, usage);
        gb.normal_view = ImageViewBuilder(gpu->device, gb.normal)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();

        gb.albedo = gpu->allocator->create_image_2D(width, height, false,
                                                     fmt, usage);
        gb.albedo_view = ImageViewBuilder(gpu->device, gb.albedo)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();

        gb.ao = gpu->allocator->create_image_2D(width, height, false,
                                                 fmt, usage);
        gb.ao_view = ImageViewBuilder(gpu->device, gb.ao)
                         .setImageType(vk::ImageViewType::e2D)
                         .build();

        gb.tangent = gpu->allocator->create_image_2D(width, height, false,
                                                      fmt, usage);
        gb.tangent_view = ImageViewBuilder(gpu->device, gb.tangent)
                              .setImageType(vk::ImageViewType::e2D)
                              .build();

        gb.bitangent = gpu->allocator->create_image_2D(width, height, false,
                                                        fmt, usage);
        gb.bitangent_view = ImageViewBuilder(gpu->device, gb.bitangent)
                                .setImageType(vk::ImageViewType::e2D)
                                .build();

        return gb;
    }

    // Build orthonormal basis from normal (Frisvad's method)
    static void build_basis(glm::vec3 N, glm::vec3 &T, glm::vec3 &B) {
        if (N.z < -0.999999f) {
            T = glm::vec3(0.0f, -1.0f, 0.0f);
            B = glm::vec3(-1.0f, 0.0f, 0.0f);
        } else {
            float a = 1.0f / (1.0f + N.z);
            float b = -N.x * N.y * a;
            T = glm::vec3(1.0f - N.x * N.x * a, b, -N.x);
            B = glm::vec3(b, 1.0f - N.y * N.y * a, -N.y);
        }
    }

    // Fill G-buffer with uniform values across all pixels
    void fill_gbuffer_uniform(GBuffer &gb, glm::vec3 position,
                              glm::vec3 normal,
                              glm::vec3 albedo = glm::vec3(1.0f),
                              float ao = 1.0f) {
        uint32_t width = gb.position->extent2D().width;
        uint32_t height = gb.position->extent2D().height;
        size_t pixel_count = width * height;
        size_t float4_size = pixel_count * 4 * sizeof(float);

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto position_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto normal_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto albedo_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto ao_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto tangent_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);
        auto bitangent_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);

        glm::vec3 n = glm::normalize(normal);
        glm::vec3 t, b;
        build_basis(n, t, b);

        std::vector<float> pos_data(pixel_count * 4);
        std::vector<float> norm_data(pixel_count * 4);
        std::vector<float> alb_data(pixel_count * 4);
        std::vector<float> ao_data(pixel_count * 4);
        std::vector<float> tan_data(pixel_count * 4);
        std::vector<float> bitan_data(pixel_count * 4);

        for (size_t i = 0; i < pixel_count; ++i) {
            pos_data[i * 4 + 0] = position.x;
            pos_data[i * 4 + 1] = position.y;
            pos_data[i * 4 + 2] = position.z;
            pos_data[i * 4 + 3] = 1.0f;

            norm_data[i * 4 + 0] = n.x;
            norm_data[i * 4 + 1] = n.y;
            norm_data[i * 4 + 2] = n.z;
            norm_data[i * 4 + 3] = 0.0f;

            alb_data[i * 4 + 0] = albedo.x;
            alb_data[i * 4 + 1] = albedo.y;
            alb_data[i * 4 + 2] = albedo.z;
            alb_data[i * 4 + 3] = 1.0f;

            ao_data[i * 4 + 0] = ao;
            ao_data[i * 4 + 1] = ao;
            ao_data[i * 4 + 2] = ao;
            ao_data[i * 4 + 3] = 1.0f;

            tan_data[i * 4 + 0] = t.x;
            tan_data[i * 4 + 1] = t.y;
            tan_data[i * 4 + 2] = t.z;
            tan_data[i * 4 + 3] = 0.0f;

            bitan_data[i * 4 + 0] = b.x;
            bitan_data[i * 4 + 1] = b.y;
            bitan_data[i * 4 + 2] = b.z;
            bitan_data[i * 4 + 3] = 0.0f;
        }

        auto write_staging = [&](auto &staging, const auto &data) {
            staging.write(
                std::span<const std::byte>(
                    reinterpret_cast<const std::byte *>(data.data()),
                    float4_size),
                0);
        };

        write_staging(position_staging, pos_data);
        write_staging(normal_staging, norm_data);
        write_staging(albedo_staging, alb_data);
        write_staging(ao_staging, ao_data);
        write_staging(tangent_staging, tan_data);
        write_staging(bitangent_staging, bitan_data);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyBufferToImage(cmd, position_staging.handle(),
                                   gb.position, 0);
        transfer.copyBufferToImage(cmd, normal_staging.handle(),
                                   gb.normal, 0);
        transfer.copyBufferToImage(cmd, albedo_staging.handle(),
                                   gb.albedo, 0);
        transfer.copyBufferToImage(cmd, ao_staging.handle(), gb.ao, 0);
        transfer.copyBufferToImage(cmd, tangent_staging.handle(),
                                   gb.tangent, 0);
        transfer.copyBufferToImage(cmd, bitangent_staging.handle(),
                                   gb.bitangent, 0);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    // Read all pixels and compute average color
    glm::vec4 read_average_color_hdr(std::shared_ptr<const Image> image) {
        uint32_t width = image->extent2D().width;
        uint32_t height = image->extent2D().height;
        size_t pixel_count = width * height;
        size_t buffer_size = pixel_count * 4 * sizeof(float);

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
        const float *data = reinterpret_cast<const float *>(pixels.data());

        glm::vec4 sum(0.0f);
        for (size_t i = 0; i < pixel_count; ++i) {
            sum.r += data[i * 4 + 0];
            sum.g += data[i * 4 + 1];
            sum.b += data[i * 4 + 2];
            sum.a += data[i * 4 + 3];
        }

        return sum / static_cast<float>(pixel_count);
    }

    // Execute the indirect light pass for N frames and return the average
    // color
    glm::vec4 run_pass(rt::RayTracedScene &scene, GBuffer &gb,
                       const SkyParameters &sky_params, Width width,
                       Height height, int num_frames = 16) {
        auto pass = std::make_unique<IndirectLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
            scene.geometry_buffer(),
            vk::Format::eR32G32B32A32Sfloat);

        std::shared_ptr<const ImageView> result;
        for (int i = 0; i < num_frames; ++i) {
            auto cmd = cmdPool->allocate(1)[0];
            std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            Barrier::ResourceTracker tracker;
            result = pass->execute(cmd, tracker, width, height,
                                   gb.position_view, gb.normal_view,
                                   gb.albedo_view, gb.ao_view,
                                   gb.tangent_view, gb.bitangent_view,
                                   sky_params);
            std::ignore = cmd.end();
            gpu->queue().enqueue_command_buffer(cmd);
            gpu->queue().submit({}, {}, {}).wait();
        }

        return read_average_color_hdr(result->image());
    }

    RayTracingGPU *gpu = nullptr;
    std::unique_ptr<CommandPool> cmdPool;
};

// =============================================================================
// Sun Bounce Tests
// =============================================================================

TEST_F(IndirectLightSunBounceTest,
       SunBounce_FloorBelowShadingPoint_ProducesNonZeroLight) {
    // A shading point facing downward (normal = 0,-1,0) above a sun-lit
    // floor should receive non-zero indirect light from the floor bouncing
    // the sun. Without sun bounce, this configuration would produce zero
    // ray-traced contribution (only ambient).
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();

    // Place a large floor below the shading point. The plane mesh is
    // centered at origin; scale it up and place at y=-5.
    auto floor_transform = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(0, -5, 0)),
        glm::vec3(100.0f, 1.0f, 100.0f));
    std::ignore = scene.add_instance(plane, floor_transform);
    scene.build();

    auto gb = create_gbuffer(width, height);
    // Shading point above the floor, facing downward
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));

    // Sun at zenith (directly above) -- floor is fully lit
    auto sky_params = SkyParameters::create_earth_sun(90.0f);

    auto color = run_pass(scene, gb, sky_params, width, height, 16);

    float luminance = color.r + color.g + color.b;

    // With sun bounce, the downward-facing surface should receive
    // light bounced from the sun-lit floor
    EXPECT_GT(luminance, 0.0f)
        << "Floor below shading point should produce non-zero bounce light"
        << " (R=" << color.r << ", G=" << color.g << ", B=" << color.b
        << ")";
}

TEST_F(IndirectLightSunBounceTest,
       SunBounce_HighSun_ProducesMoreBounceThanLowSun) {
    // A floor illuminated by a zenith sun receives more direct light
    // (higher NdotL) than one illuminated by a low-angle sun, so
    // the bounce contribution should be stronger at zenith.
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();

    auto floor_transform = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(0, -5, 0)),
        glm::vec3(100.0f, 1.0f, 100.0f));
    std::ignore = scene.add_instance(plane, floor_transform);
    scene.build();

    auto gb = create_gbuffer(width, height);
    // Shading point facing downward, above the floor
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));

    // High sun (zenith)
    auto sky_high = SkyParameters::create_earth_sun(90.0f);
    auto color_high = run_pass(scene, gb, sky_high, width, height, 16);
    float lum_high = color_high.r + color_high.g + color_high.b;

    // Low sun (near horizon)
    auto sky_low = SkyParameters::create_earth_sun(10.0f);
    auto color_low = run_pass(scene, gb, sky_low, width, height, 16);
    float lum_low = color_low.r + color_low.g + color_low.b;

    EXPECT_GT(lum_high, lum_low)
        << "Zenith sun should produce more bounce than low-angle sun"
        << " (high=" << lum_high << ", low=" << lum_low << ")";
}

TEST_F(IndirectLightSunBounceTest,
       SunBounce_OccludedFloor_ProducesLessLight) {
    // When a large occluder blocks the sun from reaching the floor,
    // the bounce contribution from the floor should be significantly
    // reduced compared to the unoccluded case.
    constexpr Width width{64};
    constexpr Height height{64};

    const auto &plane = gpu->get_plane_mesh();
    const auto &cube = gpu->get_cube_mesh();

    auto gb = create_gbuffer(width, height);
    // Shading point facing downward above the floor
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));

    // Sun at zenith
    auto sky_params = SkyParameters::create_earth_sun(90.0f);

    // Scene WITHOUT occluder
    glm::vec4 color_unoccluded;
    {
        rt::RayTracedScene scene(gpu->device, gpu->allocator);
        auto floor_transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0, -5, 0)),
            glm::vec3(100.0f, 1.0f, 100.0f));
        std::ignore = scene.add_instance(plane, floor_transform);
        scene.build();

        color_unoccluded =
            run_pass(scene, gb, sky_params, width, height, 16);
    }

    // Scene WITH large occluder between sun and floor
    glm::vec4 color_occluded;
    {
        rt::RayTracedScene scene(gpu->device, gpu->allocator);
        auto floor_transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0, -5, 0)),
            glm::vec3(100.0f, 1.0f, 100.0f));
        std::ignore = scene.add_instance(plane, floor_transform);

        // Large occluder above the floor, blocking sunlight
        auto occluder_transform = glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0, 50, 0)),
            glm::vec3(200.0f, 1.0f, 200.0f));
        std::ignore = scene.add_instance(cube, occluder_transform);
        scene.build();

        color_occluded =
            run_pass(scene, gb, sky_params, width, height, 16);
    }

    float lum_unoccluded =
        color_unoccluded.r + color_unoccluded.g + color_unoccluded.b;
    float lum_occluded =
        color_occluded.r + color_occluded.g + color_occluded.b;

    EXPECT_GT(lum_unoccluded, lum_occluded)
        << "Unoccluded floor should produce more bounce light than "
           "occluded floor"
        << " (unoccluded=" << lum_unoccluded
        << ", occluded=" << lum_occluded << ")";
}

} // namespace vw::tests
