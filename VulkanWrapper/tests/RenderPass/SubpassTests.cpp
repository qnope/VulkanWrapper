#include <gtest/gtest.h>
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "utils/create_gpu.hpp"

// Test enum with single slot
enum class SingleSlot { Output };

// Test enum with multiple slots
enum class MultiSlot { Color, Normal, Depth, Position };

// Concrete test subpass that exposes get_or_create_image for testing
template <typename SlotEnum>
class TestSubpass : public vw::Subpass<SlotEnum> {
  public:
    using vw::Subpass<SlotEnum>::Subpass;

    // Expose protected method for testing
    const vw::CachedImage &test_get_or_create_image(SlotEnum slot, vw::Width width,
                                                     vw::Height height,
                                                     size_t frame_index,
                                                     vk::Format format,
                                                     vk::ImageUsageFlags usage) {
        return this->get_or_create_image(slot, width, height, frame_index, format,
                                         usage);
    }
};

class SubpassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
    }

    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
};

TEST_F(SubpassTest, LazyAllocationCreatesImage) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    const auto &cached = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

    ASSERT_NE(cached.image, nullptr);
    ASSERT_NE(cached.view, nullptr);
    EXPECT_EQ(cached.image->extent2D().width, 256);
    EXPECT_EQ(cached.image->extent2D().height, 256);
    EXPECT_EQ(cached.image->format(), vk::Format::eR8G8B8A8Unorm);
}

TEST_F(SubpassTest, CachingReturnsSameImage) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    const auto &first = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &second = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Same pointer means same cached image
    EXPECT_EQ(first.image.get(), second.image.get());
    EXPECT_EQ(first.view.get(), second.view.get());
}

TEST_F(SubpassTest, DifferentFrameIndexCreatesDifferentImage) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    const auto &frame0 = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &frame1 = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 1,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Different frame index should create different image
    EXPECT_NE(frame0.image.get(), frame1.image.get());
    EXPECT_NE(frame0.view.get(), frame1.view.get());
}

TEST_F(SubpassTest, DimensionChangeCreatesNewImage) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    // First request at 256x256
    const auto &small = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(small.image->extent2D().width, 256);
    EXPECT_EQ(small.image->extent2D().height, 256);

    // Request at 512x512 - should create new image with new dimensions
    const auto &large = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{512}, vw::Height{512}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(large.image->extent2D().width, 512);
    EXPECT_EQ(large.image->extent2D().height, 512);

    // Request at 256x256 again - should create fresh image (old one was cleaned up)
    const auto &small_again = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(small_again.image->extent2D().width, 256);
    EXPECT_EQ(small_again.image->extent2D().height, 256);
}

TEST_F(SubpassTest, DimensionChangeRemovesOldImage) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    // Create image at 256x256 and keep a weak_ptr to track its lifetime
    const auto &small = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    std::weak_ptr<const vw::Image> weak_small = small.image;

    // Image should be alive
    EXPECT_FALSE(weak_small.expired());

    // Request different dimensions - this should remove the old image from cache
    const auto &large = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{512}, vw::Height{512}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Old image should be deleted (weak_ptr expired)
    EXPECT_TRUE(weak_small.expired());

    // New image should be valid
    EXPECT_EQ(large.image->extent2D().width, 512);
    EXPECT_EQ(large.image->extent2D().height, 512);
}

TEST_F(SubpassTest, MultipleSlots) {
    TestSubpass<MultiSlot> subpass(device, allocator);

    const auto &color = subpass.test_get_or_create_image(
        MultiSlot::Color, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &normal = subpass.test_get_or_create_image(
        MultiSlot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &depth = subpass.test_get_or_create_image(
        MultiSlot::Depth, vw::Width{256}, vw::Height{256}, 0,
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

TEST_F(SubpassTest, SlotCachingIsIndependent) {
    TestSubpass<MultiSlot> subpass(device, allocator);

    // Create color at 256x256
    const auto &color1 = subpass.test_get_or_create_image(
        MultiSlot::Color, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Create normal at 256x256
    const auto &normal1 = subpass.test_get_or_create_image(
        MultiSlot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Save normal pointer before any changes
    auto normal1_ptr = normal1.image.get();

    // Change color to 512x512 - should not affect normal
    const auto &color2 = subpass.test_get_or_create_image(
        MultiSlot::Color, vw::Width{512}, vw::Height{512}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Color should have new dimensions
    EXPECT_EQ(color2.image->extent2D().width, 512);
    EXPECT_EQ(color2.image->extent2D().height, 512);

    // Normal should still be cached at 256x256 (same pointer)
    const auto &normal2 = subpass.test_get_or_create_image(
        MultiSlot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    // Normal wasn't affected by color dimension change
    EXPECT_EQ(normal1_ptr, normal2.image.get());
    EXPECT_EQ(normal2.image->extent2D().width, 256);
}

TEST_F(SubpassTest, MultiBufferingWithMultipleSlots) {
    TestSubpass<MultiSlot> subpass(device, allocator);

    // Create images for 3 frames
    std::vector<const vw::CachedImage *> color_images;
    std::vector<const vw::CachedImage *> normal_images;

    for (size_t i = 0; i < 3; ++i) {
        color_images.push_back(&subpass.test_get_or_create_image(
            MultiSlot::Color, vw::Width{256}, vw::Height{256}, i,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eColorAttachment));

        normal_images.push_back(&subpass.test_get_or_create_image(
            MultiSlot::Normal, vw::Width{256}, vw::Height{256}, i,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment));
    }

    // All color images should be different from each other
    EXPECT_NE(color_images[0]->image.get(), color_images[1]->image.get());
    EXPECT_NE(color_images[1]->image.get(), color_images[2]->image.get());

    // All normal images should be different from each other
    EXPECT_NE(normal_images[0]->image.get(), normal_images[1]->image.get());
    EXPECT_NE(normal_images[1]->image.get(), normal_images[2]->image.get());

    // Requesting same frame again should return cached
    const auto &color0_again = subpass.test_get_or_create_image(
        MultiSlot::Color, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(color_images[0]->image.get(), color0_again.image.get());
}

TEST_F(SubpassTest, DepthFormatCreatesCorrectAspect) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    const auto &depth = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment);

    ASSERT_NE(depth.image, nullptr);
    ASSERT_NE(depth.view, nullptr);
    EXPECT_EQ(depth.image->format(), vk::Format::eD32Sfloat);
}

TEST_F(SubpassTest, NonSquareDimensions) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    const auto &wide = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{1920}, vw::Height{1080}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(wide.image->extent2D().width, 1920);
    EXPECT_EQ(wide.image->extent2D().height, 1080);
}

TEST_F(SubpassTest, VariousFormats) {
    TestSubpass<MultiSlot> subpass(device, allocator);

    const auto &rgba8 = subpass.test_get_or_create_image(
        MultiSlot::Color, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &rgba32f = subpass.test_get_or_create_image(
        MultiSlot::Normal, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    const auto &rgba16f = subpass.test_get_or_create_image(
        MultiSlot::Position, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    EXPECT_EQ(rgba8.image->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(rgba32f.image->format(), vk::Format::eR32G32B32A32Sfloat);
    EXPECT_EQ(rgba16f.image->format(), vk::Format::eR16G16B16A16Sfloat);
}

TEST_F(SubpassTest, ImageViewIsValid) {
    TestSubpass<SingleSlot> subpass(device, allocator);

    const auto &cached = subpass.test_get_or_create_image(
        SingleSlot::Output, vw::Width{256}, vw::Height{256}, 0,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);

    ASSERT_NE(cached.view, nullptr);
    EXPECT_TRUE(cached.view->handle());
    EXPECT_EQ(cached.view->image().get(), cached.image.get());
}
