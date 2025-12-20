#include "VulkanWrapper/Memory/IntervalSet.h"
#include <gtest/gtest.h>

using namespace vw;

// ============================================================================
// BufferIntervalSet Tests
// ============================================================================

TEST(BufferIntervalSetTest, AddSingleInterval) {
    BufferIntervalSet set;
    set.add(BufferInterval(100, 50));

    EXPECT_EQ(set.size(), 1);
    EXPECT_FALSE(set.empty());
    EXPECT_EQ(set.intervals()[0], BufferInterval(100, 50));
}

TEST(BufferIntervalSetTest, AddNonOverlappingIntervals) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 50));
    set.add(BufferInterval(100, 50));
    set.add(BufferInterval(200, 50));

    EXPECT_EQ(set.size(), 3);
}

TEST(BufferIntervalSetTest, AddOverlappingIntervals_Merge) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 100));
    set.add(BufferInterval(50, 100));

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0], BufferInterval(0, 150));
}

TEST(BufferIntervalSetTest, AddAdjacentIntervals_Merge) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 50));
    set.add(BufferInterval(50, 50));

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0], BufferInterval(0, 100));
}

TEST(BufferIntervalSetTest, AddMultipleOverlapping_MergeAll) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 50));
    set.add(BufferInterval(100, 50));
    set.add(BufferInterval(40, 70)); // Bridges the gap

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0], BufferInterval(0, 150));
}

TEST(BufferIntervalSetTest, HasOverlap_True) {
    BufferIntervalSet set;
    set.add(BufferInterval(100, 50));

    EXPECT_TRUE(set.hasOverlap(BufferInterval(125, 50)));
    EXPECT_TRUE(set.hasOverlap(BufferInterval(50, 100)));
}

TEST(BufferIntervalSetTest, HasOverlap_False) {
    BufferIntervalSet set;
    set.add(BufferInterval(100, 50));

    EXPECT_FALSE(set.hasOverlap(BufferInterval(0, 50)));
    EXPECT_FALSE(set.hasOverlap(BufferInterval(200, 50)));
}

TEST(BufferIntervalSetTest, FindOverlapping_Multiple) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 50));
    set.add(BufferInterval(100, 50));
    set.add(BufferInterval(200, 50));

    auto overlapping = set.findOverlapping(BufferInterval(25, 200));
    EXPECT_EQ(overlapping.size(), 3);
}

TEST(BufferIntervalSetTest, FindOverlapping_None) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 50));
    set.add(BufferInterval(100, 50));

    auto overlapping = set.findOverlapping(BufferInterval(200, 50));
    EXPECT_EQ(overlapping.size(), 0);
}

TEST(BufferIntervalSetTest, Remove_CompleteInterval) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 100));
    set.remove(BufferInterval(0, 100));

    EXPECT_TRUE(set.empty());
}

TEST(BufferIntervalSetTest, Remove_PartialFromStart) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 100));
    set.remove(BufferInterval(0, 50));

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0], BufferInterval(50, 50));
}

TEST(BufferIntervalSetTest, Remove_PartialFromEnd) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 100));
    set.remove(BufferInterval(50, 50));

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0], BufferInterval(0, 50));
}

TEST(BufferIntervalSetTest, Remove_MiddleSection_Split) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 100));
    set.remove(BufferInterval(25, 50));

    EXPECT_EQ(set.size(), 2);
    EXPECT_EQ(set.intervals()[0], BufferInterval(0, 25));
    EXPECT_EQ(set.intervals()[1], BufferInterval(75, 25));
}

TEST(BufferIntervalSetTest, Clear) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 50));
    set.add(BufferInterval(100, 50));

    EXPECT_EQ(set.size(), 2);
    set.clear();
    EXPECT_TRUE(set.empty());
}

// ============================================================================
// ImageIntervalSet Tests
// ============================================================================

TEST(ImageIntervalSetTest, AddSingleInterval) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 5;
    range.baseArrayLayer = 0;
    range.layerCount = 5;

    set.add(ImageInterval(range));

    EXPECT_EQ(set.size(), 1);
    EXPECT_FALSE(set.empty());
}

TEST(ImageIntervalSetTest, AddNonOverlappingIntervals) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange color_range;
    color_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    color_range.baseMipLevel = 0;
    color_range.levelCount = 5;
    color_range.baseArrayLayer = 0;
    color_range.layerCount = 5;

    vk::ImageSubresourceRange depth_range;
    depth_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth_range.baseMipLevel = 0;
    depth_range.levelCount = 5;
    depth_range.baseArrayLayer = 0;
    depth_range.layerCount = 5;

    set.add(ImageInterval(color_range));
    set.add(ImageInterval(depth_range));

    EXPECT_EQ(set.size(), 2);
}

TEST(ImageIntervalSetTest, AddOverlappingIntervals_Merge) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange range1;
    range1.aspectMask = vk::ImageAspectFlagBits::eColor;
    range1.baseMipLevel = 0;
    range1.levelCount = 5;
    range1.baseArrayLayer = 0;
    range1.layerCount = 5;

    vk::ImageSubresourceRange range2;
    range2.aspectMask = vk::ImageAspectFlagBits::eColor;
    range2.baseMipLevel = 3;
    range2.levelCount = 5;
    range2.baseArrayLayer = 3;
    range2.layerCount = 5;

    set.add(ImageInterval(range1));
    set.add(ImageInterval(range2));

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0].range.baseMipLevel, 0);
    EXPECT_EQ(set.intervals()[0].range.levelCount, 8);
}

TEST(ImageIntervalSetTest, HasOverlap_True) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 5;
    range.baseArrayLayer = 0;
    range.layerCount = 5;

    set.add(ImageInterval(range));

    vk::ImageSubresourceRange test_range;
    test_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    test_range.baseMipLevel = 2;
    test_range.levelCount = 5;
    test_range.baseArrayLayer = 2;
    test_range.layerCount = 5;

    EXPECT_TRUE(set.hasOverlap(ImageInterval(test_range)));
}

TEST(ImageIntervalSetTest, HasOverlap_False_DifferentAspect) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange color_range;
    color_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    color_range.baseMipLevel = 0;
    color_range.levelCount = 5;
    color_range.baseArrayLayer = 0;
    color_range.layerCount = 5;

    set.add(ImageInterval(color_range));

    vk::ImageSubresourceRange depth_range;
    depth_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth_range.baseMipLevel = 0;
    depth_range.levelCount = 5;
    depth_range.baseArrayLayer = 0;
    depth_range.layerCount = 5;

    EXPECT_FALSE(set.hasOverlap(ImageInterval(depth_range)));
}

TEST(ImageIntervalSetTest, FindOverlapping) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange range1;
    range1.aspectMask = vk::ImageAspectFlagBits::eColor;
    range1.baseMipLevel = 0;
    range1.levelCount = 5;
    range1.baseArrayLayer = 0;
    range1.layerCount = 5;

    vk::ImageSubresourceRange range2;
    range2.aspectMask = vk::ImageAspectFlagBits::eDepth;
    range2.baseMipLevel = 0;
    range2.levelCount = 5;
    range2.baseArrayLayer = 0;
    range2.layerCount = 5;

    set.add(ImageInterval(range1));
    set.add(ImageInterval(range2));

    vk::ImageSubresourceRange test_range;
    test_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    test_range.baseMipLevel = 2;
    test_range.levelCount = 5;
    test_range.baseArrayLayer = 2;
    test_range.layerCount = 5;

    auto overlapping = set.findOverlapping(ImageInterval(test_range));
    EXPECT_EQ(overlapping.size(), 1);
}

TEST(ImageIntervalSetTest, Clear) {
    ImageIntervalSet set;

    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 5;
    range.baseArrayLayer = 0;
    range.layerCount = 5;

    set.add(ImageInterval(range));
    EXPECT_EQ(set.size(), 1);

    set.clear();
    EXPECT_TRUE(set.empty());
}
