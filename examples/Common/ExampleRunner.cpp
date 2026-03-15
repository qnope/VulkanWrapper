#include "ExampleRunner.h"
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/Error.h>
#include <VulkanWrapper/Vulkan/Queue.h>

ExampleRunner::ExampleRunner(App &app) : m_app(app) {}

std::shared_ptr<const vw::ImageView> ExampleRunner::result_image() {
    return nullptr;
}

void ExampleRunner::on_resize(vw::Width /*width*/,
                               vw::Height /*height*/) {}

bool ExampleRunner::should_screenshot() const {
    return m_frame_count == 3;
}

App &ExampleRunner::app() { return m_app; }

vw::Transfer &ExampleRunner::transfer() { return m_transfer; }

int ExampleRunner::frame_count() const { return m_frame_count; }

void ExampleRunner::run() {
    try {
        setup();

        auto commandPool = vw::CommandPoolBuilder(m_app.device)
                               .with_reset_command_buffer()
                               .build();
        auto commandBuffers =
            commandPool.allocate(m_app.swapchain.number_images());

        auto imageAvailableSemaphore =
            vw::SemaphoreBuilder(m_app.device).build();
        auto renderFinishedSemaphore =
            vw::SemaphoreBuilder(m_app.device).build();

        auto do_resize = [&]() {
            auto width = m_app.window.width();
            auto height = m_app.window.height();
            if (width == vw::Width(0) ||
                height == vw::Height(0))
                return;

            m_app.device->wait_idle();
            m_app.swapchain =
                vw::SwapchainBuilder(m_app.device,
                                     m_app.surface.handle(),
                                     width, height)
                    .with_old_swapchain(m_app.swapchain.handle())
                    .build();

            commandBuffers = commandPool.allocate(
                m_app.swapchain.number_images());

            on_resize(width, height);
            m_frame_count = 0;
        };

        while (!m_app.window.is_close_requested()) {
            m_app.window.update();

            if (m_app.window.is_resized()) {
                do_resize();
                continue;
            }

            try {
                auto index =
                    m_app.swapchain.acquire_next_image(
                        imageAvailableSemaphore);

                const auto &image_view =
                    m_app.swapchain.image_views()[index];

                commandBuffers[index].reset();
                {
                    vw::CommandBufferRecorder recorder(
                        commandBuffers[index]);

                    render(commandBuffers[index], m_transfer,
                           index);

                    auto result_view = result_image();
                    if (result_view != nullptr) {
                        m_transfer.blit(commandBuffers[index],
                                        result_view->image(),
                                        image_view->image());
                    }

                    // Transition swapchain image to present
                    m_transfer.resourceTracker().request(
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
                    m_transfer.resourceTracker().flush(
                        commandBuffers[index]);
                }

                // Submit
                const vk::PipelineStageFlags waitStage =
                    vk::PipelineStageFlagBits::eTopOfPipe;
                const auto img_sem =
                    imageAvailableSemaphore.handle();
                const auto rend_sem =
                    renderFinishedSemaphore.handle();

                m_app.device->graphicsQueue()
                    .enqueue_command_buffer(
                        commandBuffers[index]);
                m_app.device->graphicsQueue().submit(
                    {&waitStage, 1}, {&img_sem, 1},
                    {&rend_sem, 1});

                m_app.swapchain.present(index,
                                        renderFinishedSemaphore);
                m_app.device->wait_idle();

                std::cout << "Iteration: " << m_frame_count
                          << std::endl;
                m_frame_count++;

                if (should_screenshot()) {
                    commandBuffers[index].reset();
                    std::ignore = commandBuffers[index].begin(
                        vk::CommandBufferBeginInfo{});

                    m_transfer.resourceTracker().request(
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
                    m_transfer.resourceTracker().flush(
                        commandBuffers[index]);

                    m_transfer.saveToFile(
                        commandBuffers[index], *m_app.allocator,
                        m_app.device->graphicsQueue(),
                        image_view->image(), "screenshot.png",
                        vk::ImageLayout::ePresentSrcKHR);

                    std::cout
                        << "Screenshot saved to screenshot.png"
                        << std::endl;
                    break;
                }

            } catch (const vw::SwapchainException &) {
                do_resize();
            }
        }

        m_app.device->wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.location().function_name()
                  << '\n';
        std::cout << exception.location().file_name() << ":"
                  << exception.location().line() << '\n';
        std::cout << "Error: " << exception.what() << '\n';
    } catch (const std::exception &e) {
        std::cout << "std::exception: " << e.what() << '\n';
    }
}
