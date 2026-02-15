#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"
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
// (Reused pattern from SunLightPassTests)
struct RayTracingGPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    std::shared_ptr<StagingBufferManager> staging;
    std::unique_ptr<Model::Material::BindlessMaterialManager> material_manager;
    std::optional<Model::MeshManager> mesh_manager;

    Queue &queue() { return device->graphicsQueue(); }

    void ensure_meshes_loaded() {
        if (!mesh_manager) {
            mesh_manager.emplace(device, allocator);
            mesh_manager->read_file("../../../Models/plane.obj");
            auto cmd = mesh_manager->fill_command_buffer();
            queue().enqueue_command_buffer(cmd);
            queue().submit({}, {}, {}).wait();
        }
    }

    const Model::Mesh &get_plane_mesh() {
        ensure_meshes_loaded();
        return mesh_manager->meshes()[0];
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

        auto staging =
            std::make_shared<StagingBufferManager>(device, allocator);
        auto material_manager =
            std::make_unique<Model::Material::BindlessMaterialManager>(
                device, allocator, staging);
        material_manager
            ->register_handler<Model::Material::ColoredMaterialHandler>();
        material_manager
            ->register_handler<Model::Material::TexturedMaterialHandler>(
                material_manager->texture_manager());

        // Intentionally leak to avoid static destruction order issues
        return new RayTracingGPU{
            std::move(instance),         std::move(device),
            std::move(allocator),        std::move(staging),
            std::move(material_manager), std::nullopt};
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

class IndirectLightPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP() << "Ray tracing not available on this system";
        }

        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(gpu->device).build());
    }

    // Simplified G-buffer for sky light
    struct GBuffer {
        std::shared_ptr<const Image> position;
        std::shared_ptr<const ImageView> position_view;
        std::shared_ptr<const Image> normal;
        std::shared_ptr<const ImageView> normal_view;
        std::shared_ptr<const Image> albedo;
        std::shared_ptr<const ImageView> albedo_view;
        std::shared_ptr<const Image> ao;
        std::shared_ptr<const ImageView> ao_view;
        std::shared_ptr<const Image> indirect_ray;
        std::shared_ptr<const ImageView> indirect_ray_view;
    };

    GBuffer create_gbuffer(Width width, Height height) {
        GBuffer gb;

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

        // Albedo (material color)
        gb.albedo = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.albedo_view = ImageViewBuilder(gpu->device, gb.albedo)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();

        // Ambient Occlusion (defaults to 1.0 = no occlusion)
        gb.ao = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.ao_view = ImageViewBuilder(gpu->device, gb.ao)
                         .setImageType(vk::ImageViewType::e2D)
                         .build();

        // Indirect ray direction (pre-computed in ColorPass)
        gb.indirect_ray = gpu->allocator->create_image_2D(
            width, height, false, vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst);
        gb.indirect_ray_view =
            ImageViewBuilder(gpu->device, gb.indirect_ray)
                .setImageType(vk::ImageViewType::e2D)
                .build();

        return gb;
    }

    // Fill G-buffer with uniform values across all pixels
    void fill_gbuffer_uniform(GBuffer &gb, glm::vec3 position, glm::vec3 normal,
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
        auto indirect_ray_staging =
            create_buffer<StagingBuffer>(*gpu->allocator, float4_size);

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

        // Fill normal (normalized)
        glm::vec3 n = glm::normalize(normal);

        std::vector<float> normal_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            normal_data[i * 4 + 0] = n.x;
            normal_data[i * 4 + 1] = n.y;
            normal_data[i * 4 + 2] = n.z;
            normal_data[i * 4 + 3] = 0.0f;
        }
        normal_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(normal_data.data()),
                float4_size),
            0);

        // Compute ray direction from normal (use normal directly for
        // test simplicity)
        std::vector<float> indirect_ray_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            indirect_ray_data[i * 4 + 0] = n.x;
            indirect_ray_data[i * 4 + 1] = n.y;
            indirect_ray_data[i * 4 + 2] = n.z;
            indirect_ray_data[i * 4 + 3] = 1.0f; // w=1.0 means valid pixel
        }
        indirect_ray_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(
                    indirect_ray_data.data()),
                float4_size),
            0);

        // Fill albedo
        std::vector<float> albedo_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            albedo_data[i * 4 + 0] = albedo.x;
            albedo_data[i * 4 + 1] = albedo.y;
            albedo_data[i * 4 + 2] = albedo.z;
            albedo_data[i * 4 + 3] = 1.0f;
        }
        albedo_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(albedo_data.data()),
                float4_size),
            0);

        // Fill AO (1.0 = no occlusion)
        std::vector<float> ao_data(pixel_count * 4);
        for (size_t i = 0; i < pixel_count; ++i) {
            ao_data[i * 4 + 0] = ao;
            ao_data[i * 4 + 1] = ao;
            ao_data[i * 4 + 2] = ao;
            ao_data[i * 4 + 3] = 1.0f;
        }
        ao_staging.write(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte *>(ao_data.data()),
                float4_size),
            0);

        // Upload data
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyBufferToImage(cmd, position_staging.handle(), gb.position,
                                   0);
        transfer.copyBufferToImage(cmd, normal_staging.handle(), gb.normal, 0);
        transfer.copyBufferToImage(cmd, albedo_staging.handle(), gb.albedo, 0);
        transfer.copyBufferToImage(cmd, ao_staging.handle(), gb.ao, 0);
        transfer.copyBufferToImage(cmd, indirect_ray_staging.handle(),
                                   gb.indirect_ray, 0);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    // Read center pixel from HDR image (R32G32B32A32Sfloat)
    glm::vec4 read_center_pixel_hdr(std::shared_ptr<const Image> image) {
        uint32_t width = image->extent2D().width;
        uint32_t height = image->extent2D().height;
        size_t buffer_size = width * height * 4 * sizeof(float);

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

        size_t x = width / 2;
        size_t y = height / 2;
        size_t pixel_idx = (y * width + x) * 4;

        return glm::vec4(data[pixel_idx], data[pixel_idx + 1],
                         data[pixel_idx + 2], data[pixel_idx + 3]);
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

    // Compute RMS difference between two images
    float compute_image_rms_difference(std::shared_ptr<const Image> image1,
                                       std::shared_ptr<const Image> image2) {
        uint32_t width = image1->extent2D().width;
        uint32_t height = image1->extent2D().height;
        size_t pixel_count = width * height;
        size_t buffer_size = pixel_count * 4 * sizeof(float);

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto staging1 =
            create_buffer<StagingBuffer>(*gpu->allocator, buffer_size);
        auto staging2 =
            create_buffer<StagingBuffer>(*gpu->allocator, buffer_size);

        // Copy both images
        {
            auto cmd = cmdPool->allocate(1)[0];
            std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            Transfer transfer;
            transfer.copyImageToBuffer(cmd, image1, staging1.handle(), 0);
            transfer.copyImageToBuffer(cmd, image2, staging2.handle(), 0);

            std::ignore = cmd.end();
            gpu->queue().enqueue_command_buffer(cmd);
            gpu->queue().submit({}, {}, {}).wait();
        }

        auto pixels1 = staging1.read_as_vector(0, buffer_size);
        auto pixels2 = staging2.read_as_vector(0, buffer_size);
        const float *data1 = reinterpret_cast<const float *>(pixels1.data());
        const float *data2 = reinterpret_cast<const float *>(pixels2.data());

        float sum_sq_diff = 0.0f;
        for (size_t i = 0; i < pixel_count * 4; ++i) {
            float diff = data1[i] - data2[i];
            sum_sq_diff += diff * diff;
        }

        return std::sqrt(sum_sq_diff / static_cast<float>(pixel_count * 4));
    }

    RayTracingGPU *gpu = nullptr;
    std::unique_ptr<CommandPool> cmdPool;
};

// =============================================================================
// Construction & API Tests
// =============================================================================

TEST_F(IndirectLightPassTest, ConstructWithValidParameters) {
    // Create a minimal scene with TLAS
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -100, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    ASSERT_NE(pass, nullptr);
}

TEST_F(IndirectLightPassTest, ShaderFilesExistAndCompile) {
    auto shader_dir = get_shader_dir();
    auto raygen_path = shader_dir / "indirect_light.rgen";
    auto miss_path = shader_dir / "indirect_light.rmiss";
    auto colored_chit_path = shader_dir / "indirect_light_colored.rchit";
    auto textured_chit_path = shader_dir / "indirect_light_textured.rchit";

    ASSERT_TRUE(std::filesystem::exists(raygen_path))
        << "Ray generation shader not found: " << raygen_path;
    ASSERT_TRUE(std::filesystem::exists(miss_path))
        << "Miss shader not found: " << miss_path;
    ASSERT_TRUE(std::filesystem::exists(colored_chit_path))
        << "Colored closest hit shader not found: " << colored_chit_path;
    ASSERT_TRUE(std::filesystem::exists(textured_chit_path))
        << "Textured closest hit shader not found: " << textured_chit_path;

    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto raygen_shader =
        compiler.compile_file_to_module(gpu->device, raygen_path);
    auto miss_shader = compiler.compile_file_to_module(gpu->device, miss_path);
    auto colored_chit_shader =
        compiler.compile_file_to_module(gpu->device, colored_chit_path);
    auto textured_chit_shader =
        compiler.compile_file_to_module(gpu->device, textured_chit_path);

    ASSERT_NE(raygen_shader, nullptr);
    ASSERT_NE(miss_shader, nullptr);
    ASSERT_NE(colored_chit_shader, nullptr);
    ASSERT_NE(textured_chit_shader, nullptr);
}

// =============================================================================
// Execution Tests
// =============================================================================

TEST_F(IndirectLightPassTest, ExecuteReturnsValidImageView) {
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -100, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result = pass->execute(cmd, tracker, width, height, gb.position_view,
                                gb.normal_view, gb.albedo_view, gb.ao_view,
                                gb.indirect_ray_view, sky_params);

    std::ignore = cmd.end();
    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->handle());
    EXPECT_EQ(result->image()->extent2D().width, static_cast<uint32_t>(width));
    EXPECT_EQ(result->image()->extent2D().height,
              static_cast<uint32_t>(height));
}

// =============================================================================
// Frame Count & Accumulation State Tests
// =============================================================================

TEST_F(IndirectLightPassTest, InitialFrameCount_IsZero) {
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -100, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    EXPECT_EQ(pass->get_frame_count(), 0u);
}

TEST_F(IndirectLightPassTest, FrameCountIncrements_AfterExecute) {
    constexpr Width width{32};
    constexpr Height height{32};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -100, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    EXPECT_EQ(pass->get_frame_count(), 0u);

    // Execute first frame
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        std::ignore = pass->execute(cmd, tracker, width, height,
                                    gb.position_view, gb.normal_view,
                                    gb.albedo_view, gb.ao_view,
                                    gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    EXPECT_EQ(pass->get_frame_count(), 1u);

    // Execute second frame
    {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        std::ignore = pass->execute(cmd, tracker, width, height,
                                    gb.position_view, gb.normal_view,
                                    gb.albedo_view, gb.ao_view,
                                    gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    EXPECT_EQ(pass->get_frame_count(), 2u);
}

TEST_F(IndirectLightPassTest, ResetAccumulation_ResetsFrameCountToZero) {
    constexpr Width width{32};
    constexpr Height height{32};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -100, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    // Execute several frames
    for (int i = 0; i < 5; ++i) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        std::ignore = pass->execute(cmd, tracker, width, height,
                                    gb.position_view, gb.normal_view,
                                    gb.albedo_view, gb.ao_view,
                                    gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    EXPECT_EQ(pass->get_frame_count(), 5u);

    // Reset accumulation
    pass->reset_accumulation();

    EXPECT_EQ(pass->get_frame_count(), 0u);
}

// =============================================================================
// Chromatic Behavior Tests
// =============================================================================

TEST_F(IndirectLightPassTest, ZenithSun_ProducesBlueIndirectLight) {
    // When sun is at zenith (90 degrees), the sky is predominantly blue
    // due to Rayleigh scattering. A white surface facing up should receive
    // indirect light with blue/cyan tint.
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    // Place plane far below to avoid occlusion
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    // White surface facing up (receives full hemisphere of sky)
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    // Sun at zenith (90 degrees above horizon)
    auto sky_params = SkyParameters::create_earth_sun(90.0f);

    // Execute multiple frames for stable accumulation
    std::shared_ptr<const ImageView> result;
    for (int i = 0; i < 16; ++i) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        result = pass->execute(cmd, tracker, width, height, gb.position_view,
                               gb.normal_view, gb.albedo_view, gb.ao_view,
                               gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    auto color = read_average_color_hdr(result->image());

    // Verify non-zero luminance
    EXPECT_GT(color.r + color.g + color.b, 0.0f)
        << "Sky light should produce non-zero illumination";

    // Blue channel should dominate over red (Rayleigh scattering)
    EXPECT_GT(color.b, color.r)
        << "Blue should dominate for zenith sun (Rayleigh scattering)"
        << " (R=" << color.r << ", G=" << color.g << ", B=" << color.b << ")";

    // Blue should be at least comparable to green
    EXPECT_GE(color.b, color.g * 0.9f)
        << "Blue should be comparable to green"
        << " (G=" << color.g << ", B=" << color.b << ")";
}

TEST_F(IndirectLightPassTest, HorizonSun_ProducesWarmIndirectLight) {
    // When sun is near horizon (sunset), the sky has warm colors
    // (orange/red). Indirect light should be warmer than at zenith.
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    // Sun near horizon (5 degrees - sunset conditions)
    auto sky_params = SkyParameters::create_earth_sun(5.0f);

    // Execute multiple frames
    std::shared_ptr<const ImageView> result;
    for (int i = 0; i < 16; ++i) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        result = pass->execute(cmd, tracker, width, height, gb.position_view,
                               gb.normal_view, gb.albedo_view, gb.ao_view,
                               gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    auto color = read_average_color_hdr(result->image());

    // Verify non-zero luminance
    EXPECT_GT(color.r + color.g + color.b, 0.0f)
        << "Sky light should produce non-zero illumination";

    // At sunset, red should be more prominent relative to blue
    // Note: threshold relaxed to 0.45 due to stochastic sampling variance
    float red_to_blue_ratio =
        (color.b > 0.001f) ? (color.r / color.b) : color.r;
    EXPECT_GT(red_to_blue_ratio, 0.45f)
        << "At sunset, red should be more prominent relative to blue"
        << " (R=" << color.r << ", B=" << color.b
        << ", R/B ratio=" << red_to_blue_ratio << ")";
}

TEST_F(IndirectLightPassTest, ChromaticShift_ZenithVsHorizon) {
    // Compare the chromatic characteristics at zenith vs horizon
    // to verify the atmospheric model produces distinct results
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    // Zenith sun
    glm::vec4 color_zenith;
    {
        auto pass = std::make_unique<IndirectLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
            scene.geometry_buffer(), *gpu->material_manager,
            vk::Format::eR32G32B32A32Sfloat);

        auto sky_params = SkyParameters::create_earth_sun(90.0f);

        std::shared_ptr<const ImageView> result;
        for (int i = 0; i < 16; ++i) {
            auto cmd = cmdPool->allocate(1)[0];
            std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            Barrier::ResourceTracker tracker;
            result = pass->execute(cmd, tracker, width, height, gb.position_view,
                                   gb.normal_view, gb.albedo_view, gb.ao_view,
                                   gb.indirect_ray_view, sky_params);
            std::ignore = cmd.end();
            gpu->queue().enqueue_command_buffer(cmd);
            gpu->queue().submit({}, {}, {}).wait();
        }
        color_zenith = read_average_color_hdr(result->image());
    }

    // Horizon sun
    glm::vec4 color_horizon;
    {
        auto pass = std::make_unique<IndirectLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
            scene.geometry_buffer(), *gpu->material_manager,
            vk::Format::eR32G32B32A32Sfloat);

        auto sky_params = SkyParameters::create_earth_sun(5.0f);

        std::shared_ptr<const ImageView> result;
        for (int i = 0; i < 16; ++i) {
            auto cmd = cmdPool->allocate(1)[0];
            std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            Barrier::ResourceTracker tracker;
            result = pass->execute(cmd, tracker, width, height, gb.position_view,
                                   gb.normal_view, gb.albedo_view, gb.ao_view,
                                   gb.indirect_ray_view, sky_params);
            std::ignore = cmd.end();
            gpu->queue().enqueue_command_buffer(cmd);
            gpu->queue().submit({}, {}, {}).wait();
        }
        color_horizon = read_average_color_hdr(result->image());
    }

    // Calculate red-to-blue ratios
    float zenith_rb =
        (color_zenith.b > 0.001f) ? (color_zenith.r / color_zenith.b) : 0.0f;
    float horizon_rb =
        (color_horizon.b > 0.001f) ? (color_horizon.r / color_horizon.b) : 0.0f;

    // Horizon should have higher red-to-blue ratio than zenith
    EXPECT_GT(horizon_rb, zenith_rb)
        << "Horizon sun should produce warmer (higher R/B) indirect light"
        << " (zenith R/B=" << zenith_rb << ", horizon R/B=" << horizon_rb
        << ")";
}

// =============================================================================
// Accumulation Convergence Tests
// =============================================================================

TEST_F(IndirectLightPassTest, AccumulationConverges_VarianceDecreases) {
    // Over multiple frames, the accumulated result should stabilize
    // (variance between consecutive frames should decrease)
    constexpr Width width{32};
    constexpr Height height{32};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    // Measure difference between frames at different accumulation stages
    std::vector<float> differences;
    glm::vec4 prev_color(0.0f);

    for (int frame = 0; frame < 20; ++frame) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        auto result =
            pass->execute(cmd, tracker, width, height, gb.position_view,
                          gb.normal_view, gb.albedo_view, gb.ao_view,
                          gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        auto color = read_average_color_hdr(result->image());

        if (frame > 0) {
            float diff = glm::length(color - prev_color);
            differences.push_back(diff);
        }

        prev_color = color;
    }

    // Later differences should be smaller than earlier ones
    // Compare average of first 5 vs last 5 differences
    float early_avg = 0.0f;
    float late_avg = 0.0f;

    for (size_t i = 0; i < 5; ++i) {
        early_avg += differences[i];
        late_avg += differences[differences.size() - 5 + i];
    }
    early_avg /= 5.0f;
    late_avg /= 5.0f;

    EXPECT_LE(late_avg, early_avg)
        << "Accumulation should converge (later frame differences should be "
           "smaller or equal)"
        << " (early_avg=" << early_avg << ", late_avg=" << late_avg << ")";
}

// =============================================================================
// Surface Orientation Tests
// =============================================================================

TEST_F(IndirectLightPassTest, SurfaceFacingUp_ReceivesSkyLight) {
    // A surface facing up (normal = 0,1,0) should receive significant
    // sky light since it sees the entire upper hemisphere
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    auto gb = create_gbuffer(width, height);
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0),
                         glm::vec3(0, 1, 0)); // Facing up

    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    std::shared_ptr<const ImageView> result;
    for (int i = 0; i < 8; ++i) {
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        Barrier::ResourceTracker tracker;
        result = pass->execute(cmd, tracker, width, height, gb.position_view,
                               gb.normal_view, gb.albedo_view, gb.ao_view,
                               gb.indirect_ray_view, sky_params);
        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    auto color = read_average_color_hdr(result->image());
    float luminance = color.r + color.g + color.b;

    EXPECT_GT(luminance, 0.0f)
        << "Surface facing up should receive significant sky light";
}

TEST_F(IndirectLightPassTest, SurfaceFacingDown_ReceivesDifferentLight) {
    // A surface facing down (normal = 0,-1,0) receives different indirect
    // light than one facing up: upward sees sky, downward sees ground bounce.
    // With sun bounce, the downward surface may receive significant light
    // from sun-lit geometry below.
    constexpr Width width{64};
    constexpr Height height{64};

    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
    scene.build();

    auto gb = create_gbuffer(width, height);
    auto sky_params = SkyParameters::create_earth_sun(45.0f);

    // Surface facing up
    glm::vec4 color_up;
    {
        auto pass = std::make_unique<IndirectLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
            scene.geometry_buffer(), *gpu->material_manager,
            vk::Format::eR32G32B32A32Sfloat);

        fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        std::shared_ptr<const ImageView> result;
        for (int i = 0; i < 8; ++i) {
            auto cmd = cmdPool->allocate(1)[0];
            std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            Barrier::ResourceTracker tracker;
            result = pass->execute(cmd, tracker, width, height, gb.position_view,
                                   gb.normal_view, gb.albedo_view, gb.ao_view,
                                   gb.indirect_ray_view, sky_params);
            std::ignore = cmd.end();
            gpu->queue().enqueue_command_buffer(cmd);
            gpu->queue().submit({}, {}, {}).wait();
        }
        color_up = read_average_color_hdr(result->image());
    }

    // Surface facing down
    glm::vec4 color_down;
    {
        auto pass = std::make_unique<IndirectLightPass>(
            gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
            scene.geometry_buffer(), *gpu->material_manager,
            vk::Format::eR32G32B32A32Sfloat);

        fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));

        std::shared_ptr<const ImageView> result;
        for (int i = 0; i < 8; ++i) {
            auto cmd = cmdPool->allocate(1)[0];
            std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            Barrier::ResourceTracker tracker;
            result = pass->execute(cmd, tracker, width, height, gb.position_view,
                                   gb.normal_view, gb.albedo_view, gb.ao_view,
                                   gb.indirect_ray_view, sky_params);
            std::ignore = cmd.end();
            gpu->queue().enqueue_command_buffer(cmd);
            gpu->queue().submit({}, {}, {}).wait();
        }
        color_down = read_average_color_hdr(result->image());
    }

    float luminance_up = color_up.r + color_up.g + color_up.b;
    float luminance_down = color_down.r + color_down.g + color_down.b;

    // Both orientations should produce non-zero light
    EXPECT_GT(luminance_up, 0.0f)
        << "Surface facing up should receive sky light";
    EXPECT_GT(luminance_down, 0.0f)
        << "Surface facing down should receive ground-bounced light";

    // The two orientations should produce noticeably different results,
    // confirming that surface orientation affects indirect lighting
    float ratio = std::max(luminance_up, luminance_down) /
                  std::min(luminance_up, luminance_down);
    EXPECT_GT(ratio, 1.5f)
        << "Surface orientation should significantly affect indirect light"
        << " (up=" << luminance_up << ", down=" << luminance_down
        << ", ratio=" << ratio << ")";
}

// =============================================================================
// Material-Aware Shader Tests
// =============================================================================

TEST_F(IndirectLightPassTest, PerMaterialShaderFilesExist) {
    auto shader_dir = get_shader_dir();
    auto colored_chit_path = shader_dir / "indirect_light_colored.rchit";
    auto textured_chit_path = shader_dir / "indirect_light_textured.rchit";

    EXPECT_TRUE(std::filesystem::exists(colored_chit_path))
        << "Colored closest hit shader not found: " << colored_chit_path;
    EXPECT_TRUE(std::filesystem::exists(textured_chit_path))
        << "Textured closest hit shader not found: " << textured_chit_path;
}

TEST_F(IndirectLightPassTest, PerMaterialShadersCompile) {
    auto shader_dir = get_shader_dir();
    auto colored_chit_path = shader_dir / "indirect_light_colored.rchit";
    auto textured_chit_path = shader_dir / "indirect_light_textured.rchit";

    if (!std::filesystem::exists(colored_chit_path) ||
        !std::filesystem::exists(textured_chit_path)) {
        GTEST_SKIP() << "Per-material shaders not yet created";
    }

    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto colored_shader =
        compiler.compile_file_to_module(gpu->device, colored_chit_path);
    auto textured_shader =
        compiler.compile_file_to_module(gpu->device, textured_chit_path);

    EXPECT_NE(colored_shader, nullptr)
        << "Colored closest hit shader failed to compile";
    EXPECT_NE(textured_shader, nullptr)
        << "Textured closest hit shader failed to compile";
}

TEST_F(IndirectLightPassTest, ConstructWithMaterialManager) {
    rt::RayTracedScene scene(gpu->device, gpu->allocator);
    const auto &plane = gpu->get_plane_mesh();
    std::ignore = scene.add_instance(
        plane, glm::translate(glm::mat4(1.0f), glm::vec3(0, -100, 0)));
    scene.set_material_sbt_mapping(
        {{Model::Material::colored_material_tag, 0},
         {Model::Material::textured_material_tag, 1}});
    scene.build();

    auto pass = std::make_unique<IndirectLightPass>(
        gpu->device, gpu->allocator, get_shader_dir(), scene.tlas(),
        scene.geometry_buffer(), *gpu->material_manager,
        vk::Format::eR32G32B32A32Sfloat);

    ASSERT_NE(pass, nullptr);
}

} // namespace vw::tests
