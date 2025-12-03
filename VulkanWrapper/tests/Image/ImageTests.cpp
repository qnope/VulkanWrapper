#include <gtest/gtest.h>
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "utils/create_gpu.hpp"

TEST(ImageTest, CreateImage2D) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{256},
        vw::Height{256},
        false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_TRUE(image->handle());
}

TEST(ImageTest, ImageExtent2D) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{256},
        false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto extent = image->extent2D();
    EXPECT_EQ(extent.width, 512);
    EXPECT_EQ(extent.height, 256);
}

TEST(ImageTest, ImageExtent3D) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{256},
        false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto extent = image->extent3D();
    EXPECT_EQ(extent.width, 512);
    EXPECT_EQ(extent.height, 256);
    EXPECT_EQ(extent.depth, 1);
}

TEST(ImageTest, CreateImageWithMipmaps) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{512},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    EXPECT_GT(static_cast<uint32_t>(image->mip_levels()), 1);
}

TEST(ImageTest, MipLevelCount) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{1024},
        vw::Height{1024},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    // 1024x1024 should have 11 mip levels (1024 -> 512 -> 256 -> 128 -> 64 -> 32 -> 16 -> 8 -> 4 -> 2 -> 1)
    EXPECT_EQ(static_cast<uint32_t>(image->mip_levels()), 11);
}

TEST(ImageTest, FullRange) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{256},
        vw::Height{256},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto range = image->full_range();
    EXPECT_EQ(range.aspectMask, vk::ImageAspectFlagBits::eColor);
    EXPECT_EQ(range.baseMipLevel, 0);
    EXPECT_EQ(range.levelCount, static_cast<uint32_t>(image->mip_levels()));
    EXPECT_EQ(range.baseArrayLayer, 0);
    EXPECT_EQ(range.layerCount, 1);
}

TEST(ImageTest, MipLevelRange) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{512},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto range = image->mip_level_range(vw::MipLevel{3});
    EXPECT_EQ(range.aspectMask, vk::ImageAspectFlagBits::eColor);
    EXPECT_EQ(range.baseMipLevel, 3);
    EXPECT_EQ(range.levelCount, 1);
}

TEST(ImageTest, MipLevelLayer) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{512},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto layer = image->mip_level_layer(vw::MipLevel{2});
    EXPECT_EQ(layer.aspectMask, vk::ImageAspectFlagBits::eColor);
    EXPECT_EQ(layer.mipLevel, 2);
    EXPECT_EQ(layer.baseArrayLayer, 0);
    EXPECT_EQ(layer.layerCount, 1);
}

TEST(ImageTest, MipLevelExtent) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{512},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);

    // Mip level 0: 512x512
    auto extent0 = image->mip_level_extent3D(vw::MipLevel{0});
    EXPECT_EQ(extent0.width, 512);
    EXPECT_EQ(extent0.height, 512);

    // Mip level 1: 256x256
    auto extent1 = image->mip_level_extent3D(vw::MipLevel{1});
    EXPECT_EQ(extent1.width, 256);
    EXPECT_EQ(extent1.height, 256);

    // Mip level 2: 128x128
    auto extent2 = image->mip_level_extent3D(vw::MipLevel{2});
    EXPECT_EQ(extent2.width, 128);
    EXPECT_EQ(extent2.height, 128);
}

TEST(ImageTest, MipLevelOffsets) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{512},
        vw::Height{512},
        true,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto offsets = image->mip_level_offsets(vw::MipLevel{1});

    EXPECT_EQ(offsets[0].x, 0);
    EXPECT_EQ(offsets[0].y, 0);
    EXPECT_EQ(offsets[0].z, 0);

    EXPECT_EQ(offsets[1].x, 256);
    EXPECT_EQ(offsets[1].y, 256);
    EXPECT_EQ(offsets[1].z, 1);
}

TEST(ImageTest, DifferentFormats) {
    auto& gpu = vw::tests::create_gpu();

    auto imageRGBA8 = gpu.allocator->create_image_2D(
        vw::Width{128}, vw::Height{128}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageRGBA16 = gpu.allocator->create_image_2D(
        vw::Width{128}, vw::Height{128}, false,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto imageDepth = gpu.allocator->create_image_2D(
        vw::Width{128}, vw::Height{128}, false,
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment
    );

    ASSERT_NE(imageRGBA8, nullptr);
    ASSERT_NE(imageRGBA16, nullptr);
    ASSERT_NE(imageDepth, nullptr);

    EXPECT_EQ(imageRGBA8->format(), vk::Format::eR8G8B8A8Unorm);
    EXPECT_EQ(imageRGBA16->format(), vk::Format::eR16G16B16A16Sfloat);
    EXPECT_EQ(imageDepth->format(), vk::Format::eD32Sfloat);
}

TEST(ImageTest, DifferentSizes) {
    auto& gpu = vw::tests::create_gpu();

    auto small = gpu.allocator->create_image_2D(
        vw::Width{64}, vw::Height{64}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto medium = gpu.allocator->create_image_2D(
        vw::Width{512}, vw::Height{512}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    auto large = gpu.allocator->create_image_2D(
        vw::Width{2048}, vw::Height{2048}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(small, nullptr);
    ASSERT_NE(medium, nullptr);
    ASSERT_NE(large, nullptr);

    EXPECT_EQ(small->extent2D().width, 64);
    EXPECT_EQ(medium->extent2D().width, 512);
    EXPECT_EQ(large->extent2D().width, 2048);
}

TEST(ImageTest, NonSquareImage) {
    auto& gpu = vw::tests::create_gpu();

    auto image = gpu.allocator->create_image_2D(
        vw::Width{1920}, vw::Height{1080}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    );

    ASSERT_NE(image, nullptr);
    auto extent = image->extent2D();
    EXPECT_EQ(extent.width, 1920);
    EXPECT_EQ(extent.height, 1080);
}
