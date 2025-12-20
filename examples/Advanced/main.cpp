#include "Application.h"
#include "DeferredRenderingManager.h"
#include "RenderPassInformation.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/Error.h>
#include <VulkanWrapper/Vulkan/Queue.h>

vw::Buffer<UBOData, true, vw::UniformBufferUsage>
createUbo(const vw::Allocator &allocator) {
    auto buffer =
        vw::create_buffer<UBOData, true, vw::UniformBufferUsage>(allocator, 1);
    UBOData data;
    buffer.write(data, 0);
    return buffer;
}

using namespace glm;

int main() {
    try {
        App app;

        auto uniform_buffer = createUbo(*app.allocator);

        // Create mesh manager and ray traced scene directly
        vw::Model::MeshManager mesh_manager(app.device, app.allocator);
        vw::rt::RayTracedScene rayTracedScene(app.device, app.allocator);

        // Load Sponza
        mesh_manager.read_file("../../../Models/Sponza/sponza.obj");
        auto sponza_mesh_count = mesh_manager.meshes().size();

        // Add all Sponza meshes with identity transform
        for (size_t i = 0; i < sponza_mesh_count; ++i) {
            std::ignore = rayTracedScene.add_instance(mesh_manager.meshes()[i],
                                                      glm::mat4(1.0f));
        }

        // Load and add a cube in the middle of the scene
        mesh_manager.read_file("../../../Models/cube.obj");

        // Create transform: scale by 300 and position in the center of Sponza
        // Sponza's courtyard is roughly at origin, raise cube slightly above
        // ground
        glm::mat4 cube_transform = glm::mat4(1.0f);
        cube_transform =
            glm::translate(cube_transform, glm::vec3(0.0f, 200.0f, 50.0f));
        cube_transform = glm::scale(cube_transform, glm::vec3(200.0f));

        // Add cube mesh (the last mesh loaded)
        for (size_t i = sponza_mesh_count; i < mesh_manager.meshes().size();
             ++i) {
            std::ignore = rayTracedScene.add_instance(mesh_manager.meshes()[i],
                                                      cube_transform);
        }

        // Upload mesh data to GPU
        auto meshUploadCmd = mesh_manager.fill_command_buffer();
        app.device->graphicsQueue().enqueue_command_buffer(meshUploadCmd);
        app.device->graphicsQueue().submit({}, {}, {}).wait();

        // Build acceleration structures for ray-traced shadows
        rayTracedScene.build();

        // Create the deferred rendering manager with functional passes
        // No swapchain needed - dimensions are passed at execute time
        DeferredRenderingManager renderingManager(app.device, app.allocator,
                                                  mesh_manager, rayTracedScene);

        // Command pool with reset support for per-frame recording
        auto commandPool = vw::CommandPoolBuilder(app.device)
                               .with_reset_command_buffer()
                               .build();
        auto commandBuffers =
            commandPool.allocate(app.swapchain.number_images());

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(app.device).build();

        // Shared Transfer across frames - ResourceTracker maintains state
        // continuity
        vw::Transfer transfer;

        int i = 0;

        // Lambda to recreate swapchain on resize
        auto recreate_swapchain = [&]() {
            auto width = app.window.width();
            auto height = app.window.height();
            if (width == vw::Width(0) || height == vw::Height(0))
                return;

            app.device->wait_idle();
            app.swapchain = vw::SwapchainBuilder(
                                app.device, app.surface.handle(), width, height)
                                .with_old_swapchain(app.swapchain.handle())
                                .build();

            commandBuffers =
                commandPool.allocate(app.swapchain.number_images());
            renderingManager.reset();

            // Update projection matrix with new aspect ratio
            float aspect =
                static_cast<float>(width) / static_cast<float>(height);
            UBOData ubo_data;
            ubo_data.proj =
                glm::perspective(glm::radians(60.0f), aspect, 2.f, 5'000.f);
            ubo_data.proj[1][1] *= -1;
            ubo_data.inverseViewProj =
                glm::inverse(ubo_data.proj * ubo_data.view);
            uniform_buffer.write(ubo_data, 0);

            i = 0;
        };

        while (!app.window.is_close_requested()) {
            app.window.update();

            // Handle explicit window resize
            if (app.window.is_resized()) {
                recreate_swapchain();
                continue;
            }

            try {
                auto index =
                    app.swapchain.acquire_next_image(imageAvailableSemaphore);

                const auto &image_view = app.swapchain.image_views()[index];

                // Reset and re-record command buffer for this frame
                // This is needed for progressive AO which updates state each
                // frame
                commandBuffers[index].reset();
                {
                    vw::CommandBufferRecorder recorder(commandBuffers[index]);

                    // Execute deferred rendering pipeline with progressive AO
                    auto light_view = renderingManager.execute(
                        commandBuffers[index], transfer.resourceTracker(),
                        app.swapchain.width(), app.swapchain.height(),
                        index, // frame_index
                        uniform_buffer,
                        90.0f, // sun_angle = zenith (90 degrees)
                        200.0f // ao_radius
                    );

                    // Blit light buffer to swapchain
                    transfer.blit(commandBuffers[index], light_view->image(),
                                  image_view->image());

                    // Transition swapchain image to present layout
                    transfer.resourceTracker().request(vw::Barrier::ImageState{
                        .image = image_view->image()->handle(),
                        .subresourceRange = image_view->subresource_range(),
                        .layout = vk::ImageLayout::ePresentSrcKHR,
                        .stage = vk::PipelineStageFlagBits2::eNone,
                        .access = vk::AccessFlagBits2::eNone});

                    transfer.resourceTracker().flush(commandBuffers[index]);
                }

                const vk::PipelineStageFlags waitStage =
                    vk::PipelineStageFlagBits::eTopOfPipe;

                const auto image_available_handle =
                    imageAvailableSemaphore.handle();
                const auto render_finished_handle =
                    renderFinishedSemaphore.handle();

                app.device->graphicsQueue().enqueue_command_buffer(
                    commandBuffers[index]);

                app.device->graphicsQueue().submit(
                    {&waitStage, 1}, {&image_available_handle, 1},
                    {&render_finished_handle, 1});

                app.swapchain.present(index, renderFinishedSemaphore);
                app.device->wait_idle();
                std::cout << "Iteration: " << i++ << std::endl;

                // Take screenshot after 16 samples and exit
                if (renderingManager.ao_pass().get_frame_count() == 16) {
                    // Record a new command buffer just for the screenshot
                    // Note: saveToFile ends the command buffer internally,
                    // so we don't use CommandBufferRecorder here
                    commandBuffers[index].reset();
                    std::ignore = commandBuffers[index].begin(
                        vk::CommandBufferBeginInfo{});

                    // Transition swapchain image to transfer src
                    transfer.resourceTracker().request(vw::Barrier::ImageState{
                        .image = image_view->image()->handle(),
                        .subresourceRange = image_view->subresource_range(),
                        .layout = vk::ImageLayout::eTransferSrcOptimal,
                        .stage = vk::PipelineStageFlagBits2::eTransfer,
                        .access = vk::AccessFlagBits2::eTransferRead});
                    transfer.resourceTracker().flush(commandBuffers[index]);

                    transfer.saveToFile(commandBuffers[index], *app.allocator,
                                        app.device->graphicsQueue(),
                                        image_view->image(), "screenshot.png",
                                        vk::ImageLayout::ePresentSrcKHR);

                    std::cout << "Screenshot saved to screenshot.png"
                              << std::endl;
                    break;
                }

            } catch (const vw::SwapchainException &) {
                // Swapchain out-of-date or suboptimal - recreate
                recreate_swapchain();
            }
        }

        app.device->wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.location().function_name() << '\n';
        std::cout << exception.location().file_name() << ":"
                  << exception.location().line() << '\n';
        std::cout << "Error: " << exception.what() << '\n';
        return 1;
    } catch (const std::exception &e) {
        std::cout << "std::exception: " << e.what() << '\n';
        return 1;
    }
}
