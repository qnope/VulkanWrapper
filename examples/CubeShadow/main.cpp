#include "Application.h"
#include "Common/SceneSetup.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/RenderPass/DirectLightPass.h>
#include <VulkanWrapper/RenderPass/RenderPipeline.h>
#include <VulkanWrapper/RenderPass/Slot.h>
#include <VulkanWrapper/RenderPass/SkyParameters.h>
#include <VulkanWrapper/RenderPass/SkyPass.h>
#include <VulkanWrapper/RenderPass/ToneMappingPass.h>
#include <VulkanWrapper/RenderPass/ZPass.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/Error.h>
#include <VulkanWrapper/Vulkan/Queue.h>

int main() {
    try {
        App app;

        // Create mesh manager and ray traced scene
        vw::Model::MeshManager mesh_manager(app.device,
                                            app.allocator);
        vw::rt::RayTracedScene rayTracedScene(app.device,
                                              app.allocator);

        // Load cube + plane scene
        auto scene_config = create_cube_plane_scene(mesh_manager);
        auto camera = scene_config.camera;

        // Create UBO with camera configuration
        float aspect =
            static_cast<float>(app.swapchain.width()) /
            static_cast<float>(app.swapchain.height());
        auto uniform_buffer =
            create_ubo(*app.allocator, aspect, camera);

        // Upload meshes, build acceleration structures
        setup_ray_tracing(mesh_manager, rayTracedScene,
                          *app.device, scene_config.instances);

        // Track initial texture states after staging
        vw::Transfer transfer;
        for (const auto &resource :
             mesh_manager.material_manager().get_resources()) {
            transfer.resourceTracker().track(resource);
        }

        // Create the render pipeline
        const std::filesystem::path shader_dir =
            "../../../VulkanWrapper/Shaders";

        vw::RenderPipeline pipeline;

        auto &zpass = pipeline.add(std::make_unique<vw::ZPass>(
            app.device, app.allocator, shader_dir));

        auto &directLight =
            pipeline.add(std::make_unique<vw::DirectLightPass>(
                app.device, app.allocator, shader_dir,
                rayTracedScene,
                mesh_manager.material_manager()));

        auto &skyPass = pipeline.add(
            std::make_unique<vw::SkyPass>(app.device,
                                          app.allocator,
                                          shader_dir));

        // No IndirectLightPass, so indirect_enabled = false
        auto &toneMapping =
            pipeline.add(std::make_unique<vw::ToneMappingPass>(
                app.device, app.allocator, shader_dir,
                vk::Format::eB8G8R8A8Srgb, false));

        auto result = pipeline.validate();
        if (!result.valid) {
            for (const auto &error : result.errors) {
                std::cout << "Pipeline validation error: "
                          << error << std::endl;
            }
            return 1;
        }

        // Command pool with reset support for per-frame recording
        auto commandPool =
            vw::CommandPoolBuilder(app.device)
                .with_reset_command_buffer()
                .build();
        auto commandBuffers =
            commandPool.allocate(app.swapchain.number_images());

        auto renderFinishedSemaphore =
            vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore =
            vw::SemaphoreBuilder(app.device).build();

        // Sun at 45° above horizon, oriented so shadow falls
        // toward camera (positive X/Z)
        auto sky_params =
            vw::SkyParameters::create_earth_sun(45.0f);
        sky_params.star_direction = glm::normalize(
            glm::vec3(0.5f, -0.707f, 0.5f));

        int frame = 0;

        while (!app.window.is_close_requested()) {
            app.window.update();

            if (app.window.is_resized()) {
                auto width = app.window.width();
                auto height = app.window.height();
                if (width == vw::Width(0) ||
                    height == vw::Height(0))
                    continue;

                app.device->wait_idle();
                app.swapchain =
                    vw::SwapchainBuilder(app.device,
                                         app.surface.handle(),
                                         width, height)
                        .with_old_swapchain(
                            app.swapchain.handle())
                        .build();
                commandBuffers = commandPool.allocate(
                    app.swapchain.number_images());
                pipeline.reset_accumulation();

                float new_aspect = static_cast<float>(width) /
                                   static_cast<float>(height);
                auto ubo_data = UBOData::create(
                    new_aspect, camera.view_matrix());
                uniform_buffer.write(ubo_data, 0);
                frame = 0;
                continue;
            }

            try {
                auto index = app.swapchain.acquire_next_image(
                    imageAvailableSemaphore);
                const auto &image_view =
                    app.swapchain.image_views()[index];

                commandBuffers[index].reset();
                {
                    vw::CommandBufferRecorder recorder(
                        commandBuffers[index]);

                    // Read UBO data for camera position
                    auto ubo_data =
                        uniform_buffer.read_as_vector(0, 1)[0];
                    glm::vec3 camera_pos = glm::vec3(
                        glm::inverse(ubo_data.view)[3]);

                    // Configure passes
                    zpass.set_uniform_buffer(uniform_buffer);
                    zpass.set_scene(rayTracedScene);

                    directLight.set_uniform_buffer(
                        uniform_buffer);
                    directLight.set_sky_parameters(sky_params);
                    directLight.set_camera_position(camera_pos);
                    directLight.set_frame_count(0);

                    skyPass.set_sky_parameters(sky_params);
                    skyPass.set_inverse_view_proj(
                        ubo_data.inverseViewProj);

                    toneMapping.set_operator(
                        vw::ToneMappingOperator::ACES);
                    toneMapping.set_exposure(1.0f);
                    toneMapping.set_white_point(4.0f);
                    toneMapping.set_luminance_scale(10000.0f);
                    toneMapping.set_indirect_intensity(0.0f);

                    // Execute the render pipeline
                    pipeline.execute(
                        commandBuffers[index],
                        transfer.resourceTracker(),
                        app.swapchain.width(),
                        app.swapchain.height(), index);

                    // Blit tone-mapped output to swapchain
                    auto tone_results =
                        toneMapping.result_images();
                    std::shared_ptr<const vw::ImageView>
                        tone_mapped_view;
                    for (const auto &[slot, cached] :
                         tone_results) {
                        if (slot == vw::Slot::ToneMapped) {
                            tone_mapped_view = cached.view;
                            break;
                        }
                    }

                    transfer.blit(commandBuffers[index],
                                  tone_mapped_view->image(),
                                  image_view->image());

                    // Transition to present
                    transfer.resourceTracker().request(
                        vw::Barrier::ImageState{
                            .image =
                                image_view->image()->handle(),
                            .subresourceRange =
                                image_view->subresource_range(),
                            .layout =
                                vk::ImageLayout::ePresentSrcKHR,
                            .stage =
                                vk::PipelineStageFlagBits2::eNone,
                            .access =
                                vk::AccessFlagBits2::eNone});
                    transfer.resourceTracker().flush(
                        commandBuffers[index]);
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
                    {&waitStage, 1},
                    {&image_available_handle, 1},
                    {&render_finished_handle, 1});

                app.swapchain.present(index,
                                      renderFinishedSemaphore);
                app.device->wait_idle();
                std::cout << "Frame: " << frame << std::endl;

                // Take screenshot at frame 3
                if (frame == 3) {
                    commandBuffers[index].reset();
                    std::ignore = commandBuffers[index].begin(
                        vk::CommandBufferBeginInfo{});

                    transfer.resourceTracker().request(
                        vw::Barrier::ImageState{
                            .image =
                                image_view->image()->handle(),
                            .subresourceRange =
                                image_view->subresource_range(),
                            .layout =
                                vk::ImageLayout::
                                    eTransferSrcOptimal,
                            .stage =
                                vk::PipelineStageFlagBits2::
                                    eTransfer,
                            .access =
                                vk::AccessFlagBits2::
                                    eTransferRead});
                    transfer.resourceTracker().flush(
                        commandBuffers[index]);

                    transfer.saveToFile(
                        commandBuffers[index], *app.allocator,
                        app.device->graphicsQueue(),
                        image_view->image(), "screenshot.png",
                        vk::ImageLayout::ePresentSrcKHR);

                    std::cout
                        << "Screenshot saved to screenshot.png"
                        << std::endl;
                    break;
                }

                ++frame;
            } catch (const vw::SwapchainException &) {
                auto width = app.window.width();
                auto height = app.window.height();
                if (width == vw::Width(0) ||
                    height == vw::Height(0))
                    continue;

                app.device->wait_idle();
                app.swapchain =
                    vw::SwapchainBuilder(app.device,
                                         app.surface.handle(),
                                         width, height)
                        .with_old_swapchain(
                            app.swapchain.handle())
                        .build();
                commandBuffers = commandPool.allocate(
                    app.swapchain.number_images());
                pipeline.reset_accumulation();

                float new_aspect = static_cast<float>(width) /
                                   static_cast<float>(height);
                auto ubo_data = UBOData::create(
                    new_aspect, camera.view_matrix());
                uniform_buffer.write(ubo_data, 0);
                frame = 0;
            }
        }

        app.device->wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.location().function_name()
                  << '\n';
        std::cout << exception.location().file_name() << ":"
                  << exception.location().line() << '\n';
        std::cout << "Error: " << exception.what() << '\n';
        return 1;
    } catch (const std::exception &e) {
        std::cout << "std::exception: " << e.what() << '\n';
        return 1;
    }
}
