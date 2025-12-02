#include <gtest/gtest.h>
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "utils/create_gpu.hpp"

TEST(ImageViewTest, CreateImageView) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageView = vw::ImageViewBuilder(gpu.device, image).build();

    ASSERT_NE(imageView, nullptr);
    EXPECT_TRUE(imageView->handle());
}

TEST(ImageViewTest, ImageViewImage) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageView = vw::ImageViewBuilder(gpu.device, image).build();

    ASSERT_NE(imageView, nullptr);
    EXPECT_EQ(imageView->image(), image);
}

TEST(ImageViewTest, ImageViewSubresourceRange) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageView = vw::ImageViewBuilder(gpu.device, image).build();

    ASSERT_NE(imageView, nullptr);
    auto range = imageView->subresource_range();
    EXPECT_EQ(range.aspectMask, vk::ImageAspectFlagBits::eColor);
}

TEST(ImageViewTest, ImageViewWithMipmaps) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{512}, vw::Height{512}, true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageView = vw::ImageViewBuilder(gpu.device, image).build();

    ASSERT_NE(imageView, nullptr);
    auto range = imageView->subresource_range();
    EXPECT_GT(range.levelCount, 1);
}

TEST(ImageViewTest, ImageView2D) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageView = vw::ImageViewBuilder(gpu.device, image)
                         .setImageType(vk::ImageViewType::e2D)
                         .build();

    ASSERT_NE(imageView, nullptr);
    EXPECT_TRUE(imageView->handle());
}

TEST(ImageViewTest, MultipleImageViews) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageView1 = vw::ImageViewBuilder(gpu.device, image).build();
    auto imageView2 = vw::ImageViewBuilder(gpu.device, image).build();

    ASSERT_NE(imageView1, nullptr);
    ASSERT_NE(imageView2, nullptr);
    EXPECT_EQ(imageView1->image(), imageView2->image());
}

TEST(ImageViewTest, DepthImageView) {
    auto gpu = vw::tests::create_gpu();

    auto image = gpu.allocator.create_image_2D(
        vw::Width{256}, vw::Height{256}, false,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment
    );

    auto imageView = vw::ImageViewBuilder(gpu.device, image).build();

    ASSERT_NE(imageView, nullptr);
    auto range = imageView->subresource_range();
    EXPECT_EQ(range.aspectMask, vk::ImageAspectFlagBits::eDepth);
}

TEST(ImageViewTest, DifferentFormatsImageViews) {
    auto gpu = vw::tests::create_gpu();

    auto imageRGBA = gpu.allocator.create_image_2D(
        vw::Width{128}, vw::Height{128}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageFloat = gpu.allocator.create_image_2D(
        vw::Width{128}, vw::Height{128}, false,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto viewRGBA = vw::ImageViewBuilder(gpu.device, imageRGBA).build();
    auto viewFloat = vw::ImageViewBuilder(gpu.device, imageFloat).build();

    ASSERT_NE(viewRGBA, nullptr);
    ASSERT_NE(viewFloat, nullptr);

    EXPECT_EQ(viewRGBA->image()->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(viewFloat->image()->format(), vk::Format::eR16G16B16A16Sfloat);
}
