#include "Application.h"
#include "DeferredRenderingManager.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/ImageLoader.h>
#include <VulkanWrapper/Image/Sampler.h>
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Model/Importer.h>
#include <VulkanWrapper/Model/Material/ColoredMaterialManager.h>
#include <VulkanWrapper/Model/Material/TexturedMaterialManager.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Model/Scene.h>
#include <VulkanWrapper/Pipeline/MeshRenderer.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h>
#include <VulkanWrapper/RayTracing/ShaderBindingTable.h>
#include <VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h>
#include <VulkanWrapper/RenderPass/Rendering.h>
#include <VulkanWrapper/RenderPass/Subpass.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Queue.h>

class VulkanExample {
  public:
    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
    vw::Swapchain &swapchain;

    vw::Queue queue = device->graphicsQueue();
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
    vw::Model::Scene scene;

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

    VulkanExample(std::shared_ptr<vw::Device> device,
                  std::shared_ptr<vw::Allocator> allocator,
                  vw::Swapchain &swapchain)
        : device{std::move(device)}
        , allocator{std::move(allocator)}
        , swapchain(swapchain)
        , m_blasList(this->device, this->allocator) {
        projectionMatrix =
            glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 512.f);
        projectionMatrix[1][1] *= -1;

        viewMatrix =
            glm::lookAt(glm::vec3(.0f, .0f, 2.f), glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));
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
        storageImage.image = allocator->create_image_2D(
            swapchain.width(), swapchain.height(), false,
            vk::Format::eR32G32B32A32Sfloat,
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
            device->handle()
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
                device->handle().allocateDescriptorSets(allocInfo)->front();

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

            device->handle().updateDescriptorSets(writeDescriptorSets, nullptr);
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
            device->handle()
                .createDescriptorSetLayoutUnique(descriptorSetlayoutCI)
                .value;

        vk::PipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.setSetLayouts(*descriptorSetLayout);
        pipelineLayout =
            device->handle().createPipelineLayoutUnique(pipelineLayoutCI).value;

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
            vw::create_buffer<UniformData, true, vw::UniformBufferUsage>(
                *allocator, 1);
        uniformBuffer->copy(std::span(&uniformData, 1), 0);
    }

    void updateUniformBuffers() {
        uniformData.projInverse = glm::inverse(projectionMatrix);
        uniformData.viewInverse = glm::inverse(viewMatrix);
        uniformBuffer->copy(std::span(&uniformData, 1), 0);
    }

    void createMeshManager() {
        mesh_manager.emplace(device, allocator);

        // Load all models first (to avoid vector reallocation invalidating
        // pointers)
        mesh_manager->read_file("../../../Models/plane.obj");
        size_t planeCount = mesh_manager->meshes().size();
        mesh_manager->read_file("../../../Models/cube.obj");

        // Now add mesh instances to the scene (vector won't resize anymore)
        // Plane at origin (already at y=0 in obj)
        for (size_t i = 0; i < planeCount; ++i) {
            scene.add_mesh_instance(mesh_manager->meshes()[i], glm::mat4(1.0f));
        }

        // Cube floating at (0, 2, 0)
        for (size_t i = planeCount; i < mesh_manager->meshes().size(); ++i) {
            glm::mat4 transform =
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
            scene.add_mesh_instance(mesh_manager->meshes()[i], transform);
        }

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
create_image_views(std::shared_ptr<const vw::Device> device,
                   const vw::Swapchain &swapchain) {
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
createUbo(const vw::Allocator &allocator) {
    auto buffer =
        vw::create_buffer<UBOData, true, vw::UniformBufferUsage>(allocator, 1);
    UBOData data;
    buffer.copy(data, 0);
    return buffer;
}

using namespace glm;

int main() {
    try {
        App app;

        auto uniform_buffer = createUbo(*app.allocator);

        VulkanExample example(app.device, app.allocator, app.swapchain);
        example.prepare();

        // Create the deferred rendering manager - handles all pass setup
        DeferredRenderingManager renderingManager(
            app.device, app.allocator, app.swapchain, *example.mesh_manager,
            example.scene, uniform_buffer);

        auto commandPool = vw::CommandPoolBuilder(app.device).build();
        auto image_views = create_image_views(app.device, app.swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        const vk::Extent2D extent(uint32_t(app.swapchain.width()),
                                  uint32_t(app.swapchain.height()));

        const auto &gBuffers = renderingManager.gbuffers();
        const auto &renderings = renderingManager.renderings();

        for (auto [gBuffer, commandBuffer, swapchainBuffer, rendering] :
             std::views::zip(gBuffers, commandBuffers, image_views,
                             renderings)) {
            vw::CommandBufferRecorder recorder(commandBuffer);

            // Use Transfer's resource tracker for the entire command buffer
            vw::Transfer transfer;

            // Geometry Pass using Rendering
            rendering.execute(commandBuffer, transfer.resourceTracker());

            // Blit color to swapchain
            transfer.blit(commandBuffer, gBuffer.light->image(),
                          swapchainBuffer->image());

            // Transition swapchain image to present layout
            transfer.resourceTracker().request(vw::Barrier::ImageState{
                .image = swapchainBuffer->image()->handle(),
                .subresourceRange = swapchainBuffer->subresource_range(),
                .layout = vk::ImageLayout::ePresentSrcKHR,
                .stage = vk::PipelineStageFlagBits2::eNone,
                .access = vk::AccessFlagBits2::eNone});

            transfer.resourceTracker().flush(commandBuffer);
        }

        auto cmd_buffer = example.mesh_manager->fill_command_buffer();

        app.device->graphicsQueue().enqueue_command_buffer(cmd_buffer);

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(app.device).build();

        bool imageSaved = false;

        while (!app.window.is_close_requested()) {
            app.window.update();

            auto index =
                app.swapchain.acquire_next_image(imageAvailableSemaphore);

            const vk::PipelineStageFlags waitStage =
                vk::PipelineStageFlagBits::eTopOfPipe;

            const auto image_available_handle =
                imageAvailableSemaphore.handle();
            const auto render_finished_handle =
                renderFinishedSemaphore.handle();

            app.device->graphicsQueue().enqueue_command_buffer(
                commandBuffers[index]);

            app.device->graphicsQueue().submit({&waitStage, 1},
                                               {&image_available_handle, 1},
                                               {&render_finished_handle, 1});

            // Save the first frame to disk
            if (!imageSaved) {
                app.device->wait_idle();

                auto saveCommandPool =
                    vw::CommandPoolBuilder(app.device).build();
                auto saveCmd = saveCommandPool.allocate(1)[0];

                std::ignore =
                    saveCmd.begin(vk::CommandBufferBeginInfo().setFlags(
                        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

                vw::Transfer saveTransfer;

                // Track the swapchain image's current state (ePresentSrcKHR
                // after rendering)
                saveTransfer.resourceTracker().track(vw::Barrier::ImageState{
                    .image = app.swapchain.images()[index]->handle(),
                    .subresourceRange =
                        app.swapchain.images()[index]->full_range(),
                    .layout = vk::ImageLayout::ePresentSrcKHR,
                    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    .access = vk::AccessFlagBits2::eColorAttachmentWrite});

                saveTransfer.saveToFile(
                    saveCmd, *app.allocator, app.device->graphicsQueue(),
                    app.swapchain.images()[index], "screenshot.png");

                std::cout << "Screenshot saved to screenshot.png\n";
                imageSaved = true;
            }

            app.device->presentQueue().present(app.swapchain, index,
                                               renderFinishedSemaphore);
            app.device->wait_idle();
            // break;
        }

        app.device->wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << '\n';
    }
}
