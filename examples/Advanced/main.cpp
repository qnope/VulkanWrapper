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

        auto commandPool = vw::CommandPoolBuilder(app.device).build();
        auto image_views = create_image_views(app.device, app.swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        // Get render dimensions
        vw::Width width = static_cast<vw::Width>(app.swapchain.width());
        vw::Height height = static_cast<vw::Height>(app.swapchain.height());

        // Record command buffers - one per swapchain image
        for (size_t i = 0; i < image_views.size(); ++i) {
            vw::CommandBufferRecorder recorder(commandBuffers[i]);

            // Use Transfer's resource tracker for the entire command buffer
            vw::Transfer transfer;

            // Execute deferred rendering pipeline functionally
            // Returns the light image view (sky + sun) which we'll blit to swapchain
            auto light_view = renderingManager.execute(
                commandBuffers[i], transfer.resourceTracker(), width, height,
                i, // frame_index
                uniform_buffer,
                90.0f,  // sun_angle = zenith (90 degrees)
                32,     // num_ao_samples
                200.0f  // ao_radius
            );

            // Blit light buffer to swapchain
            transfer.blit(commandBuffers[i], light_view->image(),
                          image_views[i]->image());

            // Transition swapchain image to present layout
            transfer.resourceTracker().request(vw::Barrier::ImageState{
                .image = image_views[i]->image()->handle(),
                .subresourceRange = image_views[i]->subresource_range(),
                .layout = vk::ImageLayout::ePresentSrcKHR,
                .stage = vk::PipelineStageFlagBits2::eNone,
                .access = vk::AccessFlagBits2::eNone});

            transfer.resourceTracker().flush(commandBuffers[i]);
        }

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
            break;
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
