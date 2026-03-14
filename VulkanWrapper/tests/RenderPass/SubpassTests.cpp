#include "utils/create_gpu.hpp"
#include "VulkanWrapper/RenderPass/RenderPass.h"
#include <gtest/gtest.h>
#include <stdexcept>

// Concrete test RenderPass that exposes get_or_create_image for testing
class TestRenderPass : public vw::RenderPass {
  public:
    using vw::RenderPass::RenderPass;

    std::vector<vw::Slot> input_slots() const override { return {}; }
    std::vector<vw::Slot> output_slots() const override {
        return m_output_slots;
    }
    void execute(vk::CommandBuffer, vw::Barrier::ResourceTracker &,
                 vw::Width, vw::Height, size_t) override {}
    std::string_view name() const override { return "TestRenderPass"; }

    // Expose protected method for testing
    const vw::CachedImage &
    test_get_or_create_image(vw::Slot slot, vw::Width width,
                             vw::Height height, size_t frame_index,
                             vk::Format format,
                             vk::ImageUsageFlags usage) {
        return get_or_create_image(slot, width, height, frame_index,
                                   format, usage);
    }

    // Expose get_input for testing
    const vw::CachedImage &test_get_input(vw::Slot slot) const {
        return get_input(slot);
    }

    std::vector<vw::Slot> m_output_slots;
};

class RenderPassBaseTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
    }

    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
};

TEST_F(RenderPassBaseTest, LazyAllocationCreatesImage) {
    TestRenderPass pass(device, allocator);

    const auto &cached = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

    ASSERT_NE(cached.image, nullptr);
    ASSERT_NE(cached.view, nullptr);
    EXPECT_EQ(cached.image->extent2D().width, 256);
    EXPECT_EQ(cached.image->extent2D().height, 256);
    EXPECT_EQ(cached.image->format(), vk::Format::eR8G8B8A8Unorm);
}

TEST_F(RenderPassBaseTest, CachingReturnsSameImage) {
    TestRenderPass pass(device, allocator);

    const auto &first = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &second = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Same pointer means same cached image
    EXPECT_EQ(first.image.get(), second.image.get());
    EXPECT_EQ(first.view.get(), second.view.get());
}

TEST_F(RenderPassBaseTest, DifferentFrameIndexCreatesDifferentImage) {
    TestRenderPass pass(device, allocator);

    const auto &frame0 = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &frame1 = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 1,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Different frame index should create different image
    EXPECT_NE(frame0.image.get(), frame1.image.get());
    EXPECT_NE(frame0.view.get(), frame1.view.get());
}

TEST_F(RenderPassBaseTest, DimensionChangeCreatesNewImage) {
    TestRenderPass pass(device, allocator);

    // First request at 256x256
    const auto &small = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(small.image->extent2D().width, 256);
    EXPECT_EQ(small.image->extent2D().height, 256);

    // Request at 512x512 - should create new image with new dimensions
    const auto &large = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{512}, vw::Height{512}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(large.image->extent2D().width, 512);
    EXPECT_EQ(large.image->extent2D().height, 512);

    // Request at 256x256 again - should create fresh image (old one was
    // cleaned up)
    const auto &small_again = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(small_again.image->extent2D().width, 256);
    EXPECT_EQ(small_again.image->extent2D().height, 256);
}

TEST_F(RenderPassBaseTest, DimensionChangeRemovesOldImage) {
    TestRenderPass pass(device, allocator);

    // Create image at 256x256 and keep a weak_ptr to track its lifetime
    const auto &small = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    std::weak_ptr<const vw::Image> weak_small = small.image;

    // Image should be alive
    EXPECT_FALSE(weak_small.expired());

    // Request different dimensions - this should remove the old image
    const auto &large = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{512}, vw::Height{512}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Old image should be deleted (weak_ptr expired)
    EXPECT_TRUE(weak_small.expired());

    // New image should be valid
    EXPECT_EQ(large.image->extent2D().width, 512);
    EXPECT_EQ(large.image->extent2D().height, 512);
}

TEST_F(RenderPassBaseTest, MultipleSlots) {
    TestRenderPass pass(device, allocator);

    const auto &color = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &normal = pass.test_get_or_create_image(
        vw::Slot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &depth = pass.test_get_or_create_image(
        vw::Slot::Depth, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment);

    // Different slots should create different images
    EXPECT_NE(color.image.get(), normal.image.get());
    EXPECT_NE(color.image.get(), depth.image.get());
    EXPECT_NE(normal.image.get(), depth.image.get());

    // Verify formats
    EXPECT_EQ(color.image->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(normal.image->format(), vk::Format::eR16G16B16A16Sfloat);
    EXPECT_EQ(depth.image->format(), vk::Format::eD32Sfloat);
}

TEST_F(RenderPassBaseTest, SlotCachingIsIndependent) {
    TestRenderPass pass(device, allocator);

    // Create color at 256x256
    const auto &color1 = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Create normal at 256x256
    const auto &normal1 = pass.test_get_or_create_image(
        vw::Slot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Save normal pointer before any changes
    auto normal1_ptr = normal1.image.get();

    // Change color to 512x512 - should not affect normal
    const auto &color2 = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{512}, vw::Height{512}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Color should have new dimensions
    EXPECT_EQ(color2.image->extent2D().width, 512);
    EXPECT_EQ(color2.image->extent2D().height, 512);

    // Normal should still be cached at 256x256 (same pointer)
    const auto &normal2 = pass.test_get_or_create_image(
        vw::Slot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Normal wasn't affected by color dimension change
    EXPECT_EQ(normal1_ptr, normal2.image.get());
    EXPECT_EQ(normal2.image->extent2D().width, 256);
}

TEST_F(RenderPassBaseTest, MultiBufferingWithMultipleSlots) {
    TestRenderPass pass(device, allocator);

    // Create images for 3 frames
    std::vector<const vw::CachedImage *> color_images;
    std::vector<const vw::CachedImage *> normal_images;

    for (size_t i = 0; i < 3; ++i) {
        color_images.push_back(&pass.test_get_or_create_image(
            vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, i,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eColorAttachment));

        normal_images.push_back(&pass.test_get_or_create_image(
            vw::Slot::Normal, vw::Width{256}, vw::Height{256}, i,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment));
    }

    // All color images should be different from each other
    EXPECT_NE(color_images[0]->image.get(),
              color_images[1]->image.get());
    EXPECT_NE(color_images[1]->image.get(),
              color_images[2]->image.get());

    // All normal images should be different from each other
    EXPECT_NE(normal_images[0]->image.get(),
              normal_images[1]->image.get());
    EXPECT_NE(normal_images[1]->image.get(),
              normal_images[2]->image.get());

    // Requesting same frame again should return cached
    const auto &color0_again = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(color_images[0]->image.get(), color0_again.image.get());
}

TEST_F(RenderPassBaseTest, DepthFormatCreatesCorrectAspect) {
    TestRenderPass pass(device, allocator);

    const auto &depth = pass.test_get_or_create_image(
        vw::Slot::Depth, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment);

    ASSERT_NE(depth.image, nullptr);
    ASSERT_NE(depth.view, nullptr);
    EXPECT_EQ(depth.image->format(), vk::Format::eD32Sfloat);
}

TEST_F(RenderPassBaseTest, NonSquareDimensions) {
    TestRenderPass pass(device, allocator);

    const auto &wide = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{1920}, vw::Height{1080}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(wide.image->extent2D().width, 1920);
    EXPECT_EQ(wide.image->extent2D().height, 1080);
}

TEST_F(RenderPassBaseTest, VariousFormats) {
    TestRenderPass pass(device, allocator);

    const auto &rgba8 = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &rgba32f = pass.test_get_or_create_image(
        vw::Slot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &rgba16f = pass.test_get_or_create_image(
        vw::Slot::Position, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(rgba8.image->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(rgba32f.image->format(),
              vk::Format::eR32G32B32A32Sfloat);
    EXPECT_EQ(rgba16f.image->format(),
              vk::Format::eR16G16B16A16Sfloat);
}

TEST_F(RenderPassBaseTest, ImageViewIsValid) {
    TestRenderPass pass(device, allocator);

    const auto &cached = pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    ASSERT_NE(cached.view, nullptr);
    EXPECT_TRUE(cached.view->handle());
    EXPECT_EQ(cached.view->image().get(), cached.image.get());
}

TEST_F(RenderPassBaseTest, ResultImagesReturnsSlotTaggedPairs) {
    TestRenderPass pass(device, allocator);

    // Create images for two different slots
    pass.test_get_or_create_image(
        vw::Slot::Albedo, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    pass.test_get_or_create_image(
        vw::Slot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    auto results = pass.result_images();

    EXPECT_EQ(results.size(), 2u);

    // Check that both slots are present
    bool found_albedo = false;
    bool found_normal = false;
    for (const auto &[slot, cached] : results) {
        if (slot == vw::Slot::Albedo) {
            found_albedo = true;
            EXPECT_EQ(cached.image->format(),
                      vk::Format::eR8G8B8A8Unorm);
        }
        if (slot == vw::Slot::Normal) {
            found_normal = true;
            EXPECT_EQ(cached.image->format(),
                      vk::Format::eR16G16B16A16Sfloat);
        }
    }
    EXPECT_TRUE(found_albedo);
    EXPECT_TRUE(found_normal);
}

TEST_F(RenderPassBaseTest, GetInputMissingSlotThrows) {
    TestRenderPass pass(device, allocator);

    EXPECT_THROW(pass.test_get_input(vw::Slot::Albedo),
                 std::runtime_error);
}
