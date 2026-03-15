#include "Application.h"
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Descriptors/Vertex.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Model/Primitive.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Shader/ShaderCompiler.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/Error.h>
#include <VulkanWrapper/Vulkan/Queue.h>

struct TrianglePushConstants {
    glm::mat4 mvp;
    glm::vec3 color;
};

int main() {
    try {
        App app;

        // Create triangle geometry
        auto [vertices, indices] =
            vw::Model::create_triangle(vw::Model::PlanePrimitive::XY);

        // Upload vertex and index buffers via staging
        auto vertex_buffer =
            vw::allocate_vertex_buffer<vw::FullVertex3D, false>(
                *app.allocator, vertices.size());
        auto index_buffer =
            vw::create_buffer<uint32_t, false, vw::IndexBufferUsage>(
                *app.allocator, indices.size());

        {
            vw::StagingBufferManager staging(app.device,
                                             app.allocator);
            staging.fill_buffer(
                std::span<const vw::FullVertex3D>{vertices},
                vertex_buffer, 0);
            staging.fill_buffer(
                std::span<const uint32_t>{indices},
                index_buffer, 0);
            auto cmd = staging.fill_command_buffer();
            app.device->graphicsQueue().enqueue_command_buffer(cmd);
            app.device->graphicsQueue().submit({}, {}, {}).wait();
        }

        // Compile shaders
        vw::ShaderCompiler compiler;
        auto vert = compiler.compile_file_to_module(
            app.device,
            "../../../examples/Triangle/triangle.vert");
        auto frag = compiler.compile_file_to_module(
            app.device,
            "../../../examples/Triangle/triangle.frag");

        // Pipeline layout with push constants
        auto layout =
            vw::PipelineLayoutBuilder(app.device)
                .with_push_constant_range(
                    {vk::ShaderStageFlagBits::eAllGraphics, 0,
                     sizeof(TrianglePushConstants)})
                .build();

        // Graphics pipeline
        auto pipeline =
            vw::GraphicsPipelineBuilder(app.device, std::move(layout))
                .add_shader(vk::ShaderStageFlagBits::eVertex, vert)
                .add_shader(vk::ShaderStageFlagBits::eFragment, frag)
                .add_vertex_binding<vw::FullVertex3D>()
                .add_color_attachment(app.swapchain.format())
                .with_dynamic_viewport_scissor()
                .build();

        // Command pool + semaphores
        auto commandPool = vw::CommandPoolBuilder(app.device)
                               .with_reset_command_buffer()
                               .build();
        auto commandBuffers =
            commandPool.allocate(app.swapchain.number_images());
        auto renderFinishedSemaphore =
            vw::SemaphoreBuilder(app.device).build();
        auto imageAvailableSemaphore =
            vw::SemaphoreBuilder(app.device).build();

        // Camera: looking at XY plane triangle from +Z
        auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f));

        float aspect =
            static_cast<float>(app.swapchain.width()) /
            static_cast<float>(app.swapchain.height());
        auto proj =
            glm::perspective(glm::radians(60.0f), aspect, 0.1f,
                             10.0f);
        proj[1][1] *= -1; // Vulkan Y flip

        TrianglePushConstants pc{};
        pc.mvp = proj * view; // model is identity
        pc.color = glm::vec3(0.2f, 0.6f, 1.0f); // blue

        vw::Transfer transfer;
        int frame = 0;

        auto swapchain_width =
            static_cast<uint32_t>(app.swapchain.width());
        auto swapchain_height =
            static_cast<uint32_t>(app.swapchain.height());

        while (!app.window.is_close_requested()) {
            app.window.update();

            try {
                auto index = app.swapchain.acquire_next_image(
                    imageAvailableSemaphore);
                const auto &image_view =
                    app.swapchain.image_views()[index];

                commandBuffers[index].reset(
                    vk::CommandBufferResetFlags{});
                {
                    vw::CommandBufferRecorder recorder(
                        commandBuffers[index]);

                    // Transition swapchain image to color attachment
                    transfer.resourceTracker().request(
                        vw::Barrier::ImageState{
                            .image =
                                image_view->image()->handle(),
                            .subresourceRange =
                                image_view->subresource_range(),
                            .layout = vk::ImageLayout::
                                eColorAttachmentOptimal,
                            .stage =
                                vk::PipelineStageFlagBits2::
                                    eColorAttachmentOutput,
                            .access =
                                vk::AccessFlagBits2::
                                    eColorAttachmentWrite});
                    transfer.resourceTracker().flush(
                        commandBuffers[index]);

                    // Begin dynamic rendering
                    vk::RenderingAttachmentInfo color_attachment{};
                    color_attachment.imageView =
                        image_view->handle();
                    color_attachment.imageLayout =
                        vk::ImageLayout::eColorAttachmentOptimal;
                    color_attachment.loadOp =
                        vk::AttachmentLoadOp::eClear;
                    color_attachment.storeOp =
                        vk::AttachmentStoreOp::eStore;
                    color_attachment.clearValue.color =
                        vk::ClearColorValue{
                            std::array{0.0f, 0.0f, 0.0f, 1.0f}};

                    vk::RenderingInfo rendering_info{};
                    rendering_info.renderArea = vk::Rect2D{
                        {0, 0},
                        {swapchain_width, swapchain_height}};
                    rendering_info.layerCount = 1;
                    rendering_info.colorAttachmentCount = 1;
                    rendering_info.pColorAttachments =
                        &color_attachment;

                    commandBuffers[index].beginRendering(
                        rendering_info);

                    // Set viewport and scissor
                    vk::Viewport viewport{
                        0.0f,
                        0.0f,
                        static_cast<float>(swapchain_width),
                        static_cast<float>(swapchain_height),
                        0.0f,
                        1.0f};
                    vk::Rect2D scissor{
                        {0, 0},
                        {swapchain_width, swapchain_height}};

                    auto cmd = commandBuffers[index];
                    cmd.setViewport(0, viewport);
                    cmd.setScissor(0, scissor);

                    // Bind pipeline
                    cmd.bindPipeline(
                        vk::PipelineBindPoint::eGraphics,
                        pipeline->handle());

                    // Bind vertex buffer
                    vk::Buffer vb = vertex_buffer.handle();
                    vk::DeviceSize offset = 0;
                    cmd.bindVertexBuffers(0, vb, offset);

                    // Bind index buffer
                    cmd.bindIndexBuffer(index_buffer.handle(), 0,
                                        vk::IndexType::eUint32);

                    // Push constants
                    cmd.pushConstants<TrianglePushConstants>(
                        pipeline->layout().handle(),
                        vk::ShaderStageFlagBits::eAllGraphics, 0,
                        pc);

                    // Draw
                    cmd.drawIndexed(
                        static_cast<uint32_t>(indices.size()), 1,
                        0, 0, 0);

                    cmd.endRendering();

                    // Transition swapchain to present
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

                // Submit
                const vk::PipelineStageFlags waitStage =
                    vk::PipelineStageFlagBits::eTopOfPipe;
                const auto img_sem =
                    imageAvailableSemaphore.handle();
                const auto rend_sem =
                    renderFinishedSemaphore.handle();

                app.device->graphicsQueue().enqueue_command_buffer(
                    commandBuffers[index]);
                app.device->graphicsQueue().submit(
                    {&waitStage, 1}, {&img_sem, 1},
                    {&rend_sem, 1});

                app.swapchain.present(index,
                                      renderFinishedSemaphore);
                app.device->wait_idle();

                frame++;
                if (frame == 3) {
                    // Take screenshot
                    commandBuffers[index].reset(
                        vk::CommandBufferResetFlags{});
                    std::ignore = commandBuffers[index].begin(
                        vk::CommandBufferBeginInfo{});

                    transfer.resourceTracker().request(
                        vw::Barrier::ImageState{
                            .image =
                                image_view->image()->handle(),
                            .subresourceRange =
                                image_view->subresource_range(),
                            .layout = vk::ImageLayout::
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

                    std::cout << "Screenshot saved to "
                                 "screenshot.png"
                              << std::endl;
                    break;
                }

            } catch (const vw::SwapchainException &) {
                // Handle resize if needed
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
