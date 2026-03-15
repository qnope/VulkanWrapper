#include "Application.h"
#include "Common/SceneSetup.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Model/Material/EmissiveTexturedMaterialHandler.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Model/Primitive.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/RenderPass/AmbientOcclusionPass.h>
#include <VulkanWrapper/RenderPass/DirectLightPass.h>
#include <VulkanWrapper/RenderPass/IndirectLightPass.h>
#include <VulkanWrapper/RenderPass/RenderPipeline.h>
#include <VulkanWrapper/RenderPass/SkyParameters.h>
#include <VulkanWrapper/RenderPass/SkyPass.h>
#include <VulkanWrapper/RenderPass/Slot.h>
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
        vw::Model::MeshManager mesh_manager(app.device, app.allocator);
        vw::rt::RayTracedScene ray_traced_scene(app.device, app.allocator);

        // Ground plane: white colored material
        auto plane_index = add_colored_mesh(
            mesh_manager,
            vw::Model::create_square(vw::Model::PlanePrimitive::XZ),
            glm::vec3(0.8f));

        // Cube: emissive textured material using stained glass
        auto cube_primitives = vw::Model::create_cube();
        auto *emissive_handler =
            static_cast<vw::Model::Material::EmissiveTexturedMaterialHandler *>(
                mesh_manager.material_manager().handler(
                    vw::Model::Material::emissive_textured_material_tag));
        auto emissive_material = emissive_handler->create_material(
            "../../../Images/stained.png", 50000.0f);
        mesh_manager.add_mesh(std::move(cube_primitives.vertices),
                              std::move(cube_primitives.indices),
                              emissive_material);
        auto cube_index = mesh_manager.meshes().size() - 1;

        // Scene layout: scaled ground plane + elevated cube
        std::vector<PendingInstance> instances;
        instances.push_back(
            {plane_index, glm::scale(glm::mat4(1.0f), glm::vec3(20.0f))});
        instances.push_back(
            {cube_index,
             glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 0.0f))});

        CameraConfig camera{
            .eye = glm::vec3(5.0f, 4.0f, 5.0f),
            .target = glm::vec3(0.0f, 0.5f, 0.0f),
        };

        // Create UBO with camera matrices
        float aspect = static_cast<float>(app.swapchain.width()) /
                       static_cast<float>(app.swapchain.height());
        auto uniform_buffer = create_ubo(*app.allocator, aspect, camera);

        // Upload meshes and build acceleration structures
        setup_ray_tracing(mesh_manager, ray_traced_scene, *app.device,
                          instances);

        // Track initial texture states after staging
        vw::Transfer transfer;
        for (const auto &resource :
             mesh_manager.material_manager().get_resources()) {
            transfer.resourceTracker().track(resource);
        }

        // Create the full render pipeline
        const std::filesystem::path shader_dir =
            "../../../VulkanWrapper/Shaders";

        vw::RenderPipeline pipeline;

        auto &zpass = pipeline.add(
            std::make_unique<vw::ZPass>(app.device, app.allocator, shader_dir));

        auto &directLight = pipeline.add(std::make_unique<vw::DirectLightPass>(
            app.device, app.allocator, shader_dir, ray_traced_scene,
            mesh_manager.material_manager()));

        auto &aoPass = pipeline.add(std::make_unique<vw::AmbientOcclusionPass>(
            app.device, app.allocator, shader_dir,
            ray_traced_scene.tlas_handle()));

        auto &skyPass = pipeline.add(std::make_unique<vw::SkyPass>(
            app.device, app.allocator, shader_dir));

        auto &indirectLight =
            pipeline.add(std::make_unique<vw::IndirectLightPass>(
                app.device, app.allocator, shader_dir, ray_traced_scene.tlas(),
                ray_traced_scene.geometry_buffer(),
                mesh_manager.material_manager()));

        auto &toneMapping = pipeline.add(std::make_unique<vw::ToneMappingPass>(
            app.device, app.allocator, shader_dir));

        auto result = pipeline.validate();
        if (!result.valid) {
            for (const auto &error : result.errors) {
                std::cout << "Pipeline validation error: " << error
                          << std::endl;
            }
            return 1;
        }

        // Command pool with reset support for per-frame
        // recording
        auto commandPool = vw::CommandPoolBuilder(app.device)
                               .with_reset_command_buffer()
                               .build();
        auto commandBuffers =
            commandPool.allocate(app.swapchain.number_images());

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(app.device).build();

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
            pipeline.reset_accumulation();

            // Update projection matrix with new aspect ratio
            float new_aspect =
                static_cast<float>(width) / static_cast<float>(height);
            auto ubo_data = UBOData::create(new_aspect, camera.view_matrix());
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

                // Reset and re-record command buffer
                commandBuffers[index].reset();
                {
                    vw::CommandBufferRecorder recorder(commandBuffers[index]);

                    // Sky parameters: sun at 0.8 radians
                    // elevation
                    auto sky_params = vw::SkyParameters::create_earth_sun(0.8f);

                    // Read UBO data for camera matrices
                    auto ubo_data = uniform_buffer.read_as_vector(0, 1)[0];
                    glm::vec3 camera_pos =
                        glm::vec3(glm::inverse(ubo_data.view)[3]);

                    // Configure passes for this frame
                    zpass.set_uniform_buffer(uniform_buffer);
                    zpass.set_scene(ray_traced_scene);

                    directLight.set_uniform_buffer(uniform_buffer);
                    directLight.set_sky_parameters(sky_params);
                    directLight.set_camera_position(camera_pos);
                    directLight.set_frame_count(
                        indirectLight.get_frame_count());

                    aoPass.set_ao_radius(2.0f);

                    skyPass.set_sky_parameters(sky_params);
                    skyPass.set_inverse_view_proj(ubo_data.inverseViewProj);

                    indirectLight.set_sky_parameters(sky_params);

                    toneMapping.set_indirect_intensity(1.0f);
                    toneMapping.set_operator(vw::ToneMappingOperator::ACES);
                    toneMapping.set_exposure(1.0f);
                    toneMapping.set_white_point(4.0f);
                    toneMapping.set_luminance_scale(10000.0f);

                    // Execute the full render pipeline
                    pipeline.execute(
                        commandBuffers[index], transfer.resourceTracker(),
                        app.swapchain.width(), app.swapchain.height(), index);

                    // Blit tone-mapped output to swapchain
                    auto tone_results = toneMapping.result_images();
                    std::shared_ptr<const vw::ImageView> tone_mapped_view;
                    for (const auto &[slot, cached] : tone_results) {
                        if (slot == vw::Slot::ToneMapped) {
                            tone_mapped_view = cached.view;
                            break;
                        }
                    }

                    transfer.blit(commandBuffers[index],
                                  tone_mapped_view->image(),
                                  image_view->image());

                    // Transition swapchain image to present
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

                // Take screenshot after 256 samples
                if (aoPass.get_frame_count() == 256) {
                    commandBuffers[index].reset();
                    std::ignore = commandBuffers[index].begin(
                        vk::CommandBufferBeginInfo{});

                    // Transition swapchain image to
                    // transfer src
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

                    std::cout << "Screenshot saved to "
                                 "screenshot.png"
                              << std::endl;
                    break;
                }

            } catch (const vw::SwapchainException &) {
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
