#include "Application.h"
#include "ColorPass.h"
#include "RayTracing.h"
#include "SkyPass.h"
#include "TonemapPass.h"
#include "ZPass.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Image/ImageLoader.h>
#include <VulkanWrapper/Image/Sampler.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Model/Importer.h>
#include <VulkanWrapper/Model/Material/ColoredMaterialManager.h>
#include <VulkanWrapper/Model/Material/TexturedMaterialManager.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Pipeline/MeshRenderer.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h>
#include <VulkanWrapper/RayTracing/ShaderBindingTable.h>
#include <VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h>
#include <VulkanWrapper/RenderPass/Attachment.h>
#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Queue.h>

/*
 * Vulkan Example - Basic hardware accelerated ray tracing example
 *  * Copyright (C) 2019-2025 by Sascha Willems - www.saschawillems.de
 *  * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */

// Holds data for a ray tracing scratch buffer that is used as a temporary
// storage
struct RayTracingScratchBuffer {
    uint64_t deviceAddress = 0;
    std::optional<vw::rt::as::ScratchBuffer> handle;
};

// Ray tracing acceleration structure
struct AccelerationStructure {
    vk::UniqueAccelerationStructureKHR handle;
    uint64_t deviceAddress = 0;
    vk::DeviceMemory memory;
    std::optional<vw::rt::as::AccelerationStructureBuffer> buffer;
};

class VulkanExample {
  public:
    vw::Device &device;
    vw::Allocator &allocator;
    vw::Swapchain &swapchain;

    vw::Queue queue = device.graphicsQueue();
    vw::CommandPool pool = vw::CommandPoolBuilder(device).build();

    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR
        rayTracingPipelineProperties{};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR
        accelerationStructureFeatures{};

    vw::rt::as::BottomLevelAccelerationStructureList m_blasList;
    std::optional<vw::rt::as::TopLevelAccelerationStructure> topLevelAS;

    std::optional<vw::Buffer<vw::Vertex3D, true, vw::VertexBufferUsage>>
        vertexBuffer;
    std::optional<vw::Buffer<uint32_t, true, vw::IndexBufferUsage>> indexBuffer;

    std::optional<vw::Model::MeshManager> mesh_manager;

    uint32_t indexCount{0};

    struct StorageImage {
        std::shared_ptr<const vw::Image> image;
        std::shared_ptr<const vw::ImageView> view;
        vk::Format format;
    } storageImage{};

    struct UniformData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    } uniformData;

    using UniformBuffer = vw::Buffer<UniformData, true, vw::UniformBufferUsage>;

    std::optional<UniformBuffer> uniformBuffer;

    std::optional<vw::rt::RayTracingPipeline> pipeline;
    std::optional<vw::PipelineLayout> pipelineLayout;
    vk::UniqueDescriptorSetLayout descriptorSetLayout{};
    vk::UniqueDescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;
    std::optional<vw::CommandPool> commandPool;
    std::vector<vk::CommandBuffer> drawCmdBuffers;
    std::optional<vw::rt::ShaderBindingTable> shaderBindingTable;

    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;

    VulkanExample(vw::Device &device, vw::Allocator &allocator,
                  vw::Swapchain &swapchain)
        : device{device}
        , allocator{allocator}
        , swapchain(swapchain)
        , m_blasList(device, allocator) {
        projectionMatrix =
            glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 512.f);
        projectionMatrix[1][1] *= -1;

        viewMatrix =
            glm::lookAt(glm::vec3(.0f, .0f, 2.f), glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));
    }

    /*
            Create a scratch buffer to hold temporary data for a ray tracing
       acceleration structure
    */
    RayTracingScratchBuffer createScratchBuffer(vk::DeviceSize size) {
        RayTracingScratchBuffer scratchBuffer{};

        scratchBuffer.handle =
            allocator.create_buffer<vw::rt::as::ScratchBuffer>(size);

        scratchBuffer.deviceAddress = scratchBuffer.handle->device_address();

        return scratchBuffer;
    }

    void createAccelerationStructureBuffer(
        AccelerationStructure &accelerationStructure,
        vk::AccelerationStructureBuildSizesInfoKHR buildSizeInfo) {

        accelerationStructure.buffer =
            allocator.create_buffer<vw::rt::as::AccelerationStructureBuffer>(
                buildSizeInfo.accelerationStructureSize);
    }

    /*
            Gets the device address from a buffer that's required for some of
       the buffers used for ray tracing
    */
    uint64_t getBufferDeviceAddress(const auto &buffer) {
        return buffer->device_address();
    }

    /*
            Set up a storage image that the ray generation shader will be
       writing to
    */
    void createStorageImage() {
        storageImage.image =
            allocator.create_image_2D(swapchain.width(), swapchain.height(),
                                      false, vk::Format::eR32G32B32A32Sfloat,
                                      vk::ImageUsageFlagBits::eStorage |
                                          vk::ImageUsageFlagBits::eTransferSrc);

        storageImage.view = vw::ImageViewBuilder(device, storageImage.image)
                                .setImageType(vk::ImageViewType::e2D)
                                .build();

        vk::CommandBuffer cmdBuffer = pool.allocate(1).front();
        std::ignore = cmdBuffer.begin(vk::CommandBufferBeginInfo());
        vw::execute_image_barrier_undefined_to_general(cmdBuffer,
                                                       storageImage.image);
        std::ignore = cmdBuffer.end();
        queue.enqueue_command_buffer(cmdBuffer);
        queue.submit({}, {}, {}).wait();
    }

    /*
            Create the bottom level acceleration structure contains the scene's
       actual geometry (vertices, triangles)
    */
    void createBottomLevelAccelerationStructure() {
        std::vector<vw::Vertex3D> vertices = {
            {{1.0f, 1.0f, 0.0f}}, {{-1.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}}};

        // Setup indices
        std::vector<uint32_t> indices = {0, 1, 2};
        indexCount = static_cast<uint32_t>(indices.size());

        // Setup identity transform matrix

        vertexBuffer =
            allocator.create_buffer<vw::Vertex3D, true, vw::VertexBufferUsage>(
                vertices.size());
        vertexBuffer->copy(vertices, 0);

        indexBuffer =
            allocator.create_buffer<uint32_t, true, vw::IndexBufferUsage>(
                indices.size());
        indexBuffer->copy(indices, 0);

        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

        vertexBufferDeviceAddress.deviceAddress =
            getBufferDeviceAddress(vertexBuffer);
        indexBufferDeviceAddress.deviceAddress =
            getBufferDeviceAddress(indexBuffer);

        // Build
        vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
        accelerationStructureGeometry.geometryType =
            vk::GeometryTypeKHR::eTriangles;
        accelerationStructureGeometry.geometry.triangles.vertexFormat =
            vk::Format::eR32G32B32Sfloat;
        accelerationStructureGeometry.geometry.triangles.vertexData =
            vertexBufferDeviceAddress;
        accelerationStructureGeometry.geometry.triangles.maxVertex = 2;
        accelerationStructureGeometry.geometry.triangles.vertexStride =
            sizeof(vw::Vertex3D);
        accelerationStructureGeometry.geometry.triangles.indexType =
            vk::IndexType::eUint32;
        accelerationStructureGeometry.geometry.triangles.indexData =
            indexBufferDeviceAddress;

        vk::AccelerationStructureBuildRangeInfoKHR
            accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = 1;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;

        auto &blas = vw::rt::as::BottomLevelAccelerationStructureBuilder(device)
                         .add_mesh(mesh_manager->meshes().front())
                         .build_into(m_blasList);

        m_blasList.submit_and_wait();
    }

    /*
        The top level acceleration structure contains the scene's object
        instances
    */
    void createTopLevelAccelerationStructure() {
        // Create transformation matrix
        glm::mat4 transform =
            glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f, 2.0f, 1.0f, -3.0f, 1.0f);

        // Get BLAS device address
        vk::DeviceAddress blasAddress = m_blasList.device_addresses().back();

        // Build TLAS using the builder
        auto commandBuffer = pool.allocate(1).front();
        std::ignore = commandBuffer.begin(vk::CommandBufferBeginInfo());

        topLevelAS =
            vw::rt::as::TopLevelAccelerationStructureBuilder(device, allocator)
                .add_bottom_level_acceleration_structure_address(blasAddress,
                                                                 transform)
                .build(commandBuffer);

        std::ignore = commandBuffer.end();
        queue.enqueue_command_buffer(commandBuffer);
        queue.submit({}, {}, {}).wait();
    }

    /*
            Create the Shader Binding Tables that binds the programs and
       top-level acceleration structure

                 SBT Layout used in this sample:

                          /-----------\
                          | raygen    |
                          |-----------|
                          | miss      |
                          |-----------|
                          | hit       |
                          \-----------/

     */
    void createShaderBindingTable() {
        shaderBindingTable.emplace(allocator,
                                   pipeline->ray_generation_handle());

        shaderBindingTable->add_miss_record(pipeline->miss_handles().front(),
                                            glm::vec3(.0, .0, .0));
        shaderBindingTable->add_hit_record(
            pipeline->closest_hit_handles().front(), glm::vec3(1.0, 1.0, 0.5));
    }

    /*
            Create the descriptor sets used for the ray tracing dispatch
    */
    void createDescriptorSets() {
        // Pool
        std::vector<vk::DescriptorPoolSize> poolSizes = {
            {vk::DescriptorType::eAccelerationStructureKHR, 1},
            {vk::DescriptorType::eStorageImage, 1},
            {vk::DescriptorType::eUniformBuffer, 1}};
        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.setPoolSizes(poolSizes).setMaxSets(10);

        descriptorPool =
            device.handle()
                .createDescriptorPoolUnique(descriptorPoolCreateInfo)
                .value;

        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool(*descriptorPool)
            .setSetLayouts(*descriptorSetLayout);

        // Sets per frame, just like the buffers themselves
        // Acceleration structure and storage image does not need to be
        // duplicated per frame, we use the same for each descriptor to keep
        // things simple

        for (auto i = 0; i < 1; i++) {
            descriptorSet =
                device.handle().allocateDescriptorSets(allocInfo)->front();

            // The fragment shader needs access to the ray tracing acceleration
            // structure, so we pass it as a descriptor

            vk::WriteDescriptorSetAccelerationStructureKHR
                descriptorAccelerationStructureInfo{};
            vk::AccelerationStructureKHR handle = topLevelAS->handle();
            descriptorAccelerationStructureInfo.setAccelerationStructures(
                handle);

            vk::WriteDescriptorSet accelerationStructureWrite{};
            // The specialized acceleration structure descriptor has to be
            // chained
            accelerationStructureWrite.pNext =
                &descriptorAccelerationStructureInfo;
            accelerationStructureWrite.dstSet = descriptorSet;
            accelerationStructureWrite.dstBinding = 0;
            accelerationStructureWrite.descriptorCount = 1;
            accelerationStructureWrite.descriptorType =
                vk::DescriptorType::eAccelerationStructureKHR;

            vk::DescriptorImageInfo storageImageDescriptor{};
            storageImageDescriptor.imageView = storageImage.view->handle();
            storageImageDescriptor.imageLayout = vk::ImageLayout::eGeneral;

            vk::WriteDescriptorSet resultImageWrite;
            resultImageWrite.setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setImageInfo(storageImageDescriptor)
                .setDstSet(descriptorSet)
                .setDstBinding(1);

            vk::DescriptorBufferInfo bufferInfoDescriptor;
            bufferInfoDescriptor.setBuffer(uniformBuffer->handle())
                .setOffset(0)
                .setRange(sizeof(UniformData));

            vk::WriteDescriptorSet uniformBufferWrite;
            uniformBufferWrite.setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setBufferInfo(bufferInfoDescriptor)
                .setDstSet(descriptorSet)
                .setDstBinding(2);

            std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
                accelerationStructureWrite, resultImageWrite,
                uniformBufferWrite};

            device.handle().updateDescriptorSets(writeDescriptorSets, nullptr);
        }
    }

    /*
            Create our ray tracing pipeline
    */
    void createRayTracingPipeline() {
        vk::DescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
        accelerationStructureLayoutBinding.binding = 0;
        accelerationStructureLayoutBinding.descriptorType =
            vk::DescriptorType::eAccelerationStructureKHR;
        accelerationStructureLayoutBinding.descriptorCount = 1;
        accelerationStructureLayoutBinding.stageFlags =
            vk::ShaderStageFlagBits::eRaygenKHR;

        vk::DescriptorSetLayoutBinding resultImageLayoutBinding{};
        resultImageLayoutBinding.binding = 1;
        resultImageLayoutBinding.descriptorType =
            vk::DescriptorType::eStorageImage;
        resultImageLayoutBinding.descriptorCount = 1;
        resultImageLayoutBinding.stageFlags =
            vk::ShaderStageFlagBits::eRaygenKHR;

        vk::DescriptorSetLayoutBinding uniformBufferBinding{};
        uniformBufferBinding.binding = 2;
        uniformBufferBinding.descriptorType =
            vk::DescriptorType::eUniformBuffer;
        uniformBufferBinding.descriptorCount = 1;
        uniformBufferBinding.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;

        std::vector<vk::DescriptorSetLayoutBinding> bindings(
            {accelerationStructureLayoutBinding, resultImageLayoutBinding,
             uniformBufferBinding});

        vk::DescriptorSetLayoutCreateInfo descriptorSetlayoutCI{};
        descriptorSetlayoutCI.setBindings(bindings);
        descriptorSetLayout =
            device.handle()
                .createDescriptorSetLayoutUnique(descriptorSetlayoutCI)
                .value;

        vk::PipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.setSetLayouts(*descriptorSetLayout);
        pipelineLayout =
            device.handle().createPipelineLayoutUnique(pipelineLayoutCI).value;

        /*
                Setup ray tracing shader groups
        */
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

        auto raygen = vw::ShaderModule::create_from_spirv_file(
            device, "Shaders/RayTracing/raygen.rgen.spv");
        auto miss = vw::ShaderModule::create_from_spirv_file(
            device, "Shaders/RayTracing/miss.rmiss.spv");
        auto hit = vw::ShaderModule::create_from_spirv_file(
            device, "Shaders/RayTracing/hit.rchit.spv");

        /*
                Create the ray tracing pipeline
        */
        pipeline = vw::rt::RayTracingPipelineBuilder(device, allocator,
                                                     std::move(*pipelineLayout))
                       .set_ray_generation_shader(raygen)
                       .add_miss_shader(miss)
                       .add_closest_hit_shader(hit)
                       .build();
    }

    /*
            Create the uniform buffer used to pass matrices to the ray tracing
       ray generation shader
    */
    void createUniformBuffer() {
        uniformBuffer =
            allocator.create_buffer<UniformData, true, vw::UniformBufferUsage>(
                1);
        uniformBuffer->copy(std::span(&uniformData, 1), 0);
    }

    void updateUniformBuffers() {
        uniformData.projInverse = glm::inverse(projectionMatrix);
        uniformData.viewInverse = glm::inverse(viewMatrix);
        uniformBuffer->copy(std::span(&uniformData, 1), 0);
    }

    void createMeshManager() {
        mesh_manager.emplace(device, allocator);
        // mesh_manager.read_file("../../../Models/Sponza/sponza.obj");
        mesh_manager->read_file("../../../Models/cube.obj");
        auto cmd_buffer = mesh_manager->fill_command_buffer();
        queue.enqueue_command_buffer(cmd_buffer);
        queue.submit({}, {}, {});
    }

    void prepare() {
        // Create the acceleration structures used to render the ray traced
        // scene
        createMeshManager();
        createBottomLevelAccelerationStructure();
        createTopLevelAccelerationStructure();

        createStorageImage();
        createUniformBuffer();
        createRayTracingPipeline();
        createShaderBindingTable();
        createDescriptorSets();
        commandPool = vw::CommandPoolBuilder(device).build();
        drawCmdBuffers = commandPool->allocate(3);

        buildCommandBuffer(0);
        buildCommandBuffer(1);
        buildCommandBuffer(2);
        updateUniformBuffers();
    }

    void buildCommandBuffer(int currentImageIndex) {
        vk::CommandBuffer cmdBuffer = drawCmdBuffers[currentImageIndex];

        vk::CommandBufferBeginInfo cmdBufInfo;
        std::ignore = cmdBuffer.begin(cmdBufInfo);
        vk::ImageSubresourceRange subresourceRange = {
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        /*
                Setup the buffer regions pointing to the shaders in our shader
           binding table
        */

        const uint32_t handleSizeAligned =
            std::max(rayTracingPipelineProperties.shaderGroupHandleSize,
                     rayTracingPipelineProperties
                         .shaderGroupHandleAlignment); // aligned size

        auto raygenShaderSbtEntry = shaderBindingTable->raygen_region();
        auto missShaderSbtEntry = shaderBindingTable->miss_region();
        auto hitShaderSbtEntry = shaderBindingTable->hit_region();

        VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

        /*
                Dispatch the ray tracing commands
        */
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                               pipeline->handle());
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                     pipeline->handle_layout(), 0,
                                     descriptorSet, nullptr);

        cmdBuffer.traceRaysKHR(raygenShaderSbtEntry, missShaderSbtEntry,
                               hitShaderSbtEntry, callableShaderSbtEntry, 800,
                               600, 1);

        /*
                Copy ray tracing output to swap chain image
        */

        auto swapchainImage = swapchain.images()[currentImageIndex];

        vw::execute_image_transition(cmdBuffer, swapchainImage,
                                     vk::ImageLayout::eUndefined,
                                     vk::ImageLayout::eTransferDstOptimal);

        // Prepare ray tracing output image as transfer source
        vw::execute_image_transition(cmdBuffer, storageImage.image,
                                     vk::ImageLayout::eGeneral,
                                     vk::ImageLayout::eTransferSrcOptimal);

        vk::ImageBlit copyRegion{};
        copyRegion.setSrcSubresource(
            {vk::ImageAspectFlagBits::eColor, 0, 0, 1});
        copyRegion.srcOffsets =
            std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D(800, 600, 1)};
        copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.dstOffsets =
            std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D(800, 600, 1)};
        cmdBuffer.blitImage(
            storageImage.image->handle(), vk::ImageLayout::eTransferSrcOptimal,
            swapchainImage->handle(), vk::ImageLayout::eTransferDstOptimal,
            copyRegion, vk::Filter::eLinear);

        // Transition swap chain image back for presentation
        vw::execute_image_transition(cmdBuffer, swapchainImage,
                                     vk::ImageLayout::eTransferDstOptimal,
                                     vk::ImageLayout::ePresentSrcKHR);

        // Transition ray tracing output image back to general layout

        vw::execute_image_transition(cmdBuffer, storageImage.image,
                                     vk::ImageLayout::eTransferSrcOptimal,
                                     vk::ImageLayout::eGeneral);

        std::ignore = cmdBuffer.end();
    }
};

std::vector<std::shared_ptr<const vw::ImageView>>
create_image_views(const vw::Device &device, const vw::Swapchain &swapchain) {
    std::vector<std::shared_ptr<const vw::ImageView>> result;
    for (auto &&image : swapchain.images()) {
        auto imageView = vw::ImageViewBuilder(device, image)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();
        result.push_back(std::move(imageView));
    }
    return result;
}

vw::Buffer<UBOData, true, vw::UniformBufferUsage>
createUbo(vw::Allocator &allocator) {
    auto buffer =
        allocator.create_buffer<UBOData, true, vw::UniformBufferUsage>(1);
    UBOData data;
    buffer.copy(data, 0);
    return buffer;
}

std::vector<vw::Framebuffer>
createGBuffers(vw::Device &device, const vw::Allocator &allocator,
               const vw::IRenderPass &renderPass,
               const vw::Swapchain &swapchain,
               const std::shared_ptr<const vw::ImageView> &depth_buffer) {
    std::vector<vw::Framebuffer> framebuffers;

    constexpr auto usageFlags = vk::ImageUsageFlagBits::eColorAttachment |
                                vk::ImageUsageFlagBits::eInputAttachment |
                                vk::ImageUsageFlagBits::eSampled;

    auto create_img = [&](vk::ImageUsageFlags otherFlags = {}) {
        return allocator.create_image_2D(swapchain.width(), swapchain.height(),
                                         false, vk::Format::eR32G32B32A32Sfloat,
                                         usageFlags | otherFlags);
    };

    auto create_img_view = [&](auto img) {
        return vw::ImageViewBuilder(device, img)
            .setImageType(vk::ImageViewType::e2D)
            .build();
    };

    for (int i = 0; i < swapchain.number_images(); ++i) {
        auto img_color = allocator.create_image_2D(
            swapchain.width(), swapchain.height(), false,
            vk::Format::eR8G8B8A8Unorm, usageFlags);

        auto img_position = create_img();
        auto img_normal = create_img();
        auto img_tangeant = create_img();
        auto img_biTangeant = create_img();
        auto img_light = create_img(vk::ImageUsageFlagBits::eStorage);

        auto img_view_color = create_img_view(img_color);
        auto img_view_position = create_img_view(img_position);
        auto img_view_normal = create_img_view(img_normal);
        auto img_view_tangeant = create_img_view(img_tangeant);
        auto img_view_biTangeant = create_img_view(img_biTangeant);
        auto img_view_light = create_img_view(img_light);

        auto framebuffer =
            vw::FramebufferBuilder(device, renderPass, swapchain.width(),
                                   swapchain.height())
                .add_attachment(img_view_color)
                .add_attachment(img_view_position)
                .add_attachment(img_view_normal)
                .add_attachment(img_view_tangeant)
                .add_attachment(img_view_biTangeant)
                .add_attachment(img_view_light)
                .add_attachment(depth_buffer)
                .build();
        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}

std::vector<vw::Framebuffer> createSwapchainFramebuffer(
    vw::Device &device, const vw::IRenderPass &renderPass,
    const std::vector<std::shared_ptr<const vw::ImageView>> &image_views,
    const vw::Swapchain &swapchain) {
    std::vector<vw::Framebuffer> framebuffers;

    for (const auto &image_view : image_views) {
        auto framebuffer =
            vw::FramebufferBuilder(device, renderPass, swapchain.width(),
                                   swapchain.height())
                .add_attachment(image_view)
                .build();
        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}

using namespace glm;

int main() {
    try {
        App app;
        /*$ auto descriptor_set_layout =
            vw::DescriptorSetLayoutBuilder(app.device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                .build();

        auto uniform_buffer = createUbo(app.allocator);

        auto sampler = vw::SamplerBuilder(app.device).build();

        auto descriptor_pool =
            vw::DescriptorPoolBuilder(app.device, descriptor_set_layout)
                .build();

        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(0, uniform_buffer.handle(), 0,
                                                uniform_buffer.size_bytes());

        auto descriptor_set =
            descriptor_pool.allocate_set(descriptor_allocator);

        const auto depth_buffer = app.allocator.create_image_2D(
            app.swapchain.width(), app.swapchain.height(), false,
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eSampled);

        const auto depth_buffer_view =
            vw::ImageViewBuilder(app.device, depth_buffer)
                .setImageType(vk::ImageViewType::e2D)
                .build();

        const auto color_attachment =
            vw::AttachmentBuilder{}
                .with_format(vk::Format::eR8G8B8A8Unorm)
                .with_final_layout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .build();

        const auto data_attachment =
            vw::AttachmentBuilder{}
                .with_format(vk::Format::eR32G32B32A32Sfloat)
                .with_final_layout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .build();

        const auto light_attachment =
            vw::AttachmentBuilder{}
                .with_format(vk::Format::eR32G32B32A32Sfloat)
                .with_final_layout(vk::ImageLayout::eGeneral)
                .build();

        const auto final_attachment =
            vw::AttachmentBuilder{}
                .with_format(app.swapchain.format())
                .with_final_layout(vk::ImageLayout::ePresentSrcKHR)
                .build();

        const auto depth_attachment =
            vw::AttachmentBuilder{}
                .with_format(depth_buffer->format())
                .with_final_layout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .build();

        auto depth_subpass = std::make_unique<ZPass>(
            app.device, mesh_manager, descriptor_set_layout,
            app.swapchain.width(), app.swapchain.height(), descriptor_set);
        auto color_subpass = std::make_unique<ColorSubpass>(
            app.device, mesh_manager, descriptor_set_layout,
            app.swapchain.width(), app.swapchain.height(), descriptor_set);
        auto sky_pass = std::make_unique<SkyPass>(
            app.device, app.allocator, app.swapchain.width(),
            app.swapchain.height(), UBOData{}.proj, UBOData{}.view);
        auto sky_buffer = sky_pass->get_ubo();

        auto geometrySkyRenderPass =
            vw::RenderPassBuilder(app.device)
                .add_attachment(color_attachment,
                                vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f))
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(data_attachment, vk::ClearColorValue())
                .add_attachment(light_attachment, vk::ClearColorValue())
                .add_attachment(depth_attachment,
                                vk::ClearDepthStencilValue(1.0))
                .add_subpass(z_pass_tag, std::move(depth_subpass))
                .add_subpass(color_pass_tag, std::move(color_subpass))
                //.add_subpass(sky_pass_tag, std::move(sky_pass))
                .add_dependency(z_pass_tag, color_pass_tag)
                //.add_dependency(z_pass_tag, sky_pass_tag)
                .build<GBufferInformation>();

        auto tonemap_pass = std::make_unique<TonemapPass>(
            app.device, app.swapchain.width(), app.swapchain.height());

        auto tonemapRenderPass =
            vw::RenderPassBuilder(app.device)
                .add_attachment(final_attachment, vk::ClearColorValue())
                .add_subpass(tonemap_pass_tag, std::move(tonemap_pass))
                .build<TonemapInformation>();

        auto commandPool = vw::CommandPoolBuilder(app.device).build();
        auto image_views = create_image_views(app.device, app.swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        const auto gBuffers =
            createGBuffers(app.device, app.allocator, geometrySkyRenderPass,
                           app.swapchain, depth_buffer_view);

        const auto swapchainBuffers = createSwapchainFramebuffer(
            app.device, tonemapRenderPass, image_views, app.swapchain);

        const vk::Extent2D extent(uint32_t(app.swapchain.width()),
                                  uint32_t(app.swapchain.height()));

        RayTracingPass rayTracingPass(app.device, app.allocator, mesh_manager,
                                      app.swapchain.width(),
                                      app.swapchain.height());

        for (auto [gBuffer, commandBuffer, swapchainBuffer] :
             std::views::zip(gBuffers, commandBuffers, swapchainBuffers)) {
            vw::CommandBufferRecorder recorder(commandBuffer);
            geometrySkyRenderPass.execute(commandBuffer, gBuffer,
                                          GBufferInformation{&gBuffer});

            TonemapInformation info{
                vw::CombinedImage{gBuffer.image_view(0), sampler},
                vw::CombinedImage{gBuffer.image_view(5), sampler}};

            rayTracingPass.execute(commandBuffer, gBuffer,
                                   uniform_buffer.handle());

            tonemapRenderPass.execute(commandBuffer, swapchainBuffer, info);
        }

        auto cmd_buffer = mesh_manager.fill_command_buffer();

        app.device.graphicsQueue().enqueue_command_buffer(cmd_buffer);
        float angle = 90.0;
        */

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(app.device).build();

        VulkanExample example(app.device, app.allocator, app.swapchain);

        example.prepare();

        while (!app.window.is_close_requested()) {
            app.window.update();

            // SkyPass::UBO ubo{UBOData{}.proj, UBOData{}.view, angle};
            // sky_buffer->copy({&ubo, 1u}, 0);

            auto index =
                app.swapchain.acquire_next_image(imageAvailableSemaphore);

            const vk::PipelineStageFlags waitStage =
                vk::PipelineStageFlagBits::eTopOfPipe;

            const auto image_available_handle =
                imageAvailableSemaphore.handle();
            const auto render_finished_handle =
                renderFinishedSemaphore.handle();

            app.device.graphicsQueue().enqueue_command_buffer(
                example.drawCmdBuffers[index]);

            auto fence = app.device.graphicsQueue().submit(
                {&waitStage, 1}, {&image_available_handle, 1},
                {&render_finished_handle, 1});

            fence.wait();

            app.device.presentQueue().present(app.swapchain, index,
                                              renderFinishedSemaphore);
            app.device.wait_idle();
        }

        app.device.wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << '\n';
    }
}
