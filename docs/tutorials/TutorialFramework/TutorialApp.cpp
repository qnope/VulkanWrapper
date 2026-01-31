#include "TutorialApp.h"

#include <VulkanWrapper/Command/CommandBufferRecorder.h>
#include <VulkanWrapper/Image/ImageViewBuilder.h>
#include <VulkanWrapper/Memory/AllocatorBuilder.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Vulkan/InstanceBuilder.h>

#include <iostream>

namespace vw::tutorial {

TutorialApp::TutorialApp(TutorialConfig config) : m_config(std::move(config)) {}

TutorialApp::TutorialApp(std::string name, uint32_t width, uint32_t height)
    : m_config{.name = std::move(name), .width = width, .height = height} {}

TutorialApp::~TutorialApp() = default;

void TutorialApp::run() {
    std::cout << "=== " << m_config.name << " ===" << std::endl;
    std::cout << "Resolution: " << m_config.width << "x" << m_config.height
              << std::endl;

    // Initialize Vulkan
    std::cout << "Initializing Vulkan..." << std::endl;
    initVulkan();

    // Create render target
    std::cout << "Creating render target..." << std::endl;
    createRenderTarget();

    // User setup
    std::cout << "Setting up tutorial resources..." << std::endl;
    setup();

    // Render frames
    std::cout << "Rendering " << m_config.frameCount << " frame(s)..."
              << std::endl;
    for (uint32_t i = 0; i < m_config.frameCount; ++i) {
        renderFrame(i);
    }

    // Wait for GPU to finish
    m_device->wait_idle();

    // Save screenshot
    std::cout << "Saving screenshot to " << m_config.screenshotPath << "..."
              << std::endl;
    saveScreenshot();

    // User cleanup
    std::cout << "Cleaning up..." << std::endl;
    cleanup();

    std::cout << "Done!" << std::endl;
}

void TutorialApp::initVulkan() {
    // Create instance with validation layers
    m_instance = InstanceBuilder()
                     .setDebug()
                     .addPortability()
                     .setApiVersion(ApiVersion::e13)
                     .build();

    // Find suitable GPU with graphics queue
    m_device = m_instance->findGpu()
                   .with_queue(vk::QueueFlagBits::eGraphics)
                   .with_synchronization_2()
                   .with_dynamic_rendering()
                   .build();

    // Get graphics queue
    m_queue = &m_device->find_queue(vk::QueueFlagBits::eGraphics);

    // Create allocator
    m_allocator = AllocatorBuilder(m_instance, m_device).build();

    // Create command pool
    m_commandPool =
        std::make_unique<CommandPool>(CommandPoolBuilder(m_device).build());
    m_commandBuffers = m_commandPool->allocate(1);

    // Create fence for synchronization
    m_fence = std::make_unique<Fence>(m_device);
}

void TutorialApp::createRenderTarget() {
    // Create render target image
    m_renderTarget = m_allocator->create_image(
        m_config.colorFormat, Width{m_config.width}, Height{m_config.height},
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    // Create image view
    m_renderTargetView =
        ImageViewBuilder(m_device, m_renderTarget).as_2d().build();

    // Track initial state
    m_transfer.resourceTracker().track(Barrier::ImageState{
        .image = m_renderTarget->handle(),
        .subresourceRange = m_renderTarget->full_range(),
        .layout = vk::ImageLayout::eUndefined,
        .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
        .access = vk::AccessFlagBits2::eNone});
}

vk::RenderingAttachmentInfo TutorialApp::colorAttachment(
    vk::AttachmentLoadOp loadOp, vk::ClearColorValue clearColor) const {
    return vk::RenderingAttachmentInfo{
        .imageView = m_renderTargetView->handle(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = loadOp,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearValue{.color = clearColor}};
}

void TutorialApp::beginRendering(vk::CommandBuffer cmd,
                                 vk::AttachmentLoadOp loadOp,
                                 vk::ClearColorValue clearColor) {
    // Transition render target to color attachment
    m_transfer.resourceTracker().request(Barrier::ImageState{
        .image = m_renderTarget->handle(),
        .subresourceRange = m_renderTarget->full_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    m_transfer.resourceTracker().flush(cmd);

    auto attachment = colorAttachment(loadOp, clearColor);
    vk::RenderingInfo renderingInfo{
        .renderArea = {{0, 0}, {m_config.width, m_config.height}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment};

    cmd.beginRendering(renderingInfo);

    // Set viewport and scissor
    cmd.setViewport(
        0, vk::Viewport{0.0f, 0.0f, static_cast<float>(m_config.width),
                        static_cast<float>(m_config.height), 0.0f, 1.0f});
    cmd.setScissor(0, vk::Rect2D{{0, 0}, {m_config.width, m_config.height}});
}

void TutorialApp::endRendering(vk::CommandBuffer cmd) { cmd.endRendering(); }

void TutorialApp::renderFrame(uint32_t frameIndex) {
    auto cmd = m_commandBuffers[0];

    // Begin recording
    {
        CommandBufferRecorder recorder(cmd);

        // Call user render function
        render(cmd, frameIndex);
    }

    // Submit
    vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmd};
    vk::SubmitInfo2 submitInfo{.commandBufferInfoCount = 1,
                               .pCommandBufferInfos = &cmdInfo};

    m_fence->reset();
    m_queue->handle().submit2(submitInfo, m_fence->handle());
    m_fence->wait();
}

void TutorialApp::saveScreenshot() {
    auto cmd = m_commandBuffers[0];

    // Transition to transfer src
    m_transfer.resourceTracker().request(Barrier::ImageState{
        .image = m_renderTarget->handle(),
        .subresourceRange = m_renderTarget->full_range(),
        .layout = vk::ImageLayout::eTransferSrcOptimal,
        .stage = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferRead});

    {
        CommandBufferRecorder recorder(cmd);
        m_transfer.resourceTracker().flush(cmd);
    }

    vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmd};
    vk::SubmitInfo2 submitInfo{.commandBufferInfoCount = 1,
                               .pCommandBufferInfos = &cmdInfo};

    m_fence->reset();
    m_queue->handle().submit2(submitInfo, m_fence->handle());
    m_fence->wait();

    // Save to file
    m_transfer.saveToFile(m_commandBuffers[0], *m_allocator, *m_queue,
                          *m_renderTarget, m_config.screenshotPath);
}

}  // namespace vw::tutorial
