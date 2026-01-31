#pragma once

/**
 * TutorialApp - Base class for minimal tutorial examples
 *
 * This framework provides a simple way to create Vulkan tutorials that:
 * 1. Initialize all required Vulkan objects
 * 2. Render a single frame (or multiple for progressive rendering)
 * 3. Save a screenshot automatically
 * 4. Clean up resources
 *
 * Usage:
 *   class MyTutorial : public TutorialApp {
 *   public:
 *       MyTutorial() : TutorialApp("My Tutorial", 800, 600) {}
 *       void setup() override { // Initialize resources }
 *       void render(vk::CommandBuffer cmd, uint32_t frameIndex) override {
 *           // Render commands
 *       }
 *       void cleanup() override { // Optional cleanup }
 *   };
 *
 *   int main() {
 *       MyTutorial app;
 *       app.run();
 *       return 0;
 *   }
 */

#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Image/Image.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Memory/Transfer.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/Instance.h>

#include <cstdint>
#include <memory>
#include <string>

namespace vw::tutorial {

/**
 * Configuration for tutorial rendering
 */
struct TutorialConfig {
    std::string name = "Tutorial";
    uint32_t width = 800;
    uint32_t height = 600;
    uint32_t frameCount = 1;  // Number of frames to render before screenshot
    vk::Format colorFormat = vk::Format::eR8G8B8A8Unorm;
    std::string screenshotPath = "screenshot.png";
};

/**
 * Base class for all tutorial examples
 *
 * Provides:
 * - Vulkan instance with validation layers
 * - Device with graphics queue
 * - Memory allocator
 * - Render target image
 * - Command pool and buffers
 * - Automatic screenshot capture
 */
class TutorialApp {
public:
    explicit TutorialApp(TutorialConfig config);
    TutorialApp(std::string name, uint32_t width, uint32_t height);
    virtual ~TutorialApp();

    // Non-copyable, non-movable
    TutorialApp(const TutorialApp &) = delete;
    TutorialApp &operator=(const TutorialApp &) = delete;
    TutorialApp(TutorialApp &&) = delete;
    TutorialApp &operator=(TutorialApp &&) = delete;

    /**
     * Main entry point - initializes, renders, saves screenshot
     */
    void run();

protected:
    /**
     * Override to set up tutorial-specific resources
     * Called once after Vulkan initialization
     */
    virtual void setup() = 0;

    /**
     * Override to record render commands
     * @param cmd Command buffer in recording state
     * @param frameIndex Current frame (0 to frameCount-1)
     */
    virtual void render(vk::CommandBuffer cmd, uint32_t frameIndex) = 0;

    /**
     * Override to clean up tutorial-specific resources
     * Called before Vulkan cleanup
     */
    virtual void cleanup() {}

    // Accessors for derived classes
    [[nodiscard]] std::shared_ptr<Instance> instance() const {
        return m_instance;
    }
    [[nodiscard]] std::shared_ptr<Device> device() const { return m_device; }
    [[nodiscard]] std::shared_ptr<Allocator> allocator() const {
        return m_allocator;
    }
    [[nodiscard]] Queue &queue() const { return *m_queue; }
    [[nodiscard]] const TutorialConfig &config() const { return m_config; }

    // Render target
    [[nodiscard]] std::shared_ptr<Image> renderTarget() const {
        return m_renderTarget;
    }
    [[nodiscard]] std::shared_ptr<ImageView> renderTargetView() const {
        return m_renderTargetView;
    }

    // Resource tracking
    [[nodiscard]] Transfer &transfer() { return m_transfer; }

    // Helper: Create a rendering attachment info for the render target
    [[nodiscard]] vk::RenderingAttachmentInfo colorAttachment(
        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
        vk::ClearColorValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}}) const;

    // Helper: Begin dynamic rendering to the render target
    void beginRendering(vk::CommandBuffer cmd,
                        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
                        vk::ClearColorValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}});

    // Helper: End dynamic rendering
    void endRendering(vk::CommandBuffer cmd);

private:
    void initVulkan();
    void createRenderTarget();
    void renderFrame(uint32_t frameIndex);
    void saveScreenshot();

    TutorialConfig m_config;

    // Core Vulkan objects
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    Queue *m_queue = nullptr;

    // Command recording
    std::unique_ptr<CommandPool> m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::unique_ptr<Fence> m_fence;

    // Render target
    std::shared_ptr<Image> m_renderTarget;
    std::shared_ptr<ImageView> m_renderTargetView;

    // Resource tracking
    Transfer m_transfer;
};

}  // namespace vw::tutorial
