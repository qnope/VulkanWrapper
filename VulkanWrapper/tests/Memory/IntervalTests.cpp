#include "VulkanWrapper/Memory/Interval.h"
#include <gtest/gtest.h>

using namespace vw;

// ============================================================================
// BufferInterval Tests
// ============================================================================

TEST(BufferIntervalTest, EmptyInterval) {
    BufferInterval empty(0, 0);
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.end(), 0);
}

TEST(BufferIntervalTest, BasicProperties) {
    BufferInterval interval(100, 50);
    EXPECT_FALSE(interval.empty());
    EXPECT_EQ(interval.offset, 100);
    EXPECT_EQ(interval.size, 50);
    EXPECT_EQ(interval.end(), 150);
}

TEST(BufferIntervalTest, Contains_CompletelyInside) {
    BufferInterval outer(0, 100);
    BufferInterval inner(25, 50);
    EXPECT_TRUE(outer.contains(inner));
    EXPECT_FALSE(inner.contains(outer));
}

TEST(BufferIntervalTest, Contains_Identical) {
    BufferInterval a(50, 100);
    BufferInterval b(50, 100);
    EXPECT_TRUE(a.contains(b));
    EXPECT_TRUE(b.contains(a));
}

TEST(BufferIntervalTest, Contains_PartialOverlap) {
    BufferInterval a(0, 100);
    BufferInterval b(50, 100);
    EXPECT_FALSE(a.contains(b));
    EXPECT_FALSE(b.contains(a));
}

TEST(BufferIntervalTest, Contains_NoOverlap) {
    BufferInterval a(0, 50);
    BufferInterval b(100, 50);
    EXPECT_FALSE(a.contains(b));
    EXPECT_FALSE(b.contains(a));
}

TEST(BufferIntervalTest, Contains_Empty) {
    BufferInterval a(0, 100);
    BufferInterval empty(0, 0);
    EXPECT_FALSE(a.contains(empty));
    EXPECT_FALSE(empty.contains(a));
}

TEST(BufferIntervalTest, Overlaps_PartialOverlap) {
    BufferInterval a(0, 100);
    BufferInterval b(50, 100);
    EXPECT_TRUE(a.overlaps(b));
    EXPECT_TRUE(b.overlaps(a));
}

TEST(BufferIntervalTest, Overlaps_CompletelyInside) {
    BufferInterval outer(0, 100);
    BufferInterval inner(25, 50);
    EXPECT_TRUE(outer.overlaps(inner));
    EXPECT_TRUE(inner.overlaps(outer));
}

TEST(BufferIntervalTest, Overlaps_Adjacent) {
    BufferInterval a(0, 50);
    BufferInterval b(50, 50);
    EXPECT_FALSE(a.overlaps(b));
    EXPECT_FALSE(b.overlaps(a));
}

TEST(BufferIntervalTest, Overlaps_Separated) {
    BufferInterval a(0, 50);
    BufferInterval b(100, 50);
    EXPECT_FALSE(a.overlaps(b));
    EXPECT_FALSE(b.overlaps(a));
}

TEST(BufferIntervalTest, Overlaps_Empty) {
    BufferInterval a(0, 100);
    BufferInterval empty(0, 0);
    EXPECT_FALSE(a.overlaps(empty));
    EXPECT_FALSE(empty.overlaps(a));
}

TEST(BufferIntervalTest, Merge_Overlapping) {
    BufferInterval a(0, 100);
    BufferInterval b(50, 100);
    auto merged = a.merge(b);
    ASSERT_TRUE(merged.has_value());
    EXPECT_EQ(merged->offset, 0);
    EXPECT_EQ(merged->size, 150);
}

TEST(BufferIntervalTest, Merge_Adjacent) {
    BufferInterval a(0, 50);
    BufferInterval b(50, 50);
    auto merged = a.merge(b);
    ASSERT_TRUE(merged.has_value());
    EXPECT_EQ(merged->offset, 0);
    EXPECT_EQ(merged->size, 100);
}

TEST(BufferIntervalTest, Merge_Separated) {
    BufferInterval a(0, 50);
    BufferInterval b(100, 50);
    auto merged = a.merge(b);
    EXPECT_FALSE(merged.has_value());
}

TEST(BufferIntervalTest, Merge_Identical) {
    BufferInterval a(50, 100);
    BufferInterval b(50, 100);
    auto merged = a.merge(b);
    ASSERT_TRUE(merged.has_value());
    EXPECT_EQ(*merged, a);
}

TEST(BufferIntervalTest, Merge_WithEmpty) {
    BufferInterval a(0, 100);
    BufferInterval empty(0, 0);
    auto merged = a.merge(empty);
    ASSERT_TRUE(merged.has_value());
    EXPECT_EQ(*merged, a);
}

TEST(BufferIntervalTest, Intersect_Overlapping) {
    BufferInterval a(0, 100);
    BufferInterval b(50, 100);
    auto intersection = a.intersect(b);
    ASSERT_TRUE(intersection.has_value());
    EXPECT_EQ(intersection->offset, 50);
    EXPECT_EQ(intersection->size, 50);
}

TEST(BufferIntervalTest, Intersect_CompletelyInside) {
    BufferInterval outer(0, 100);
    BufferInterval inner(25, 50);
    auto intersection = outer.intersect(inner);
    ASSERT_TRUE(intersection.has_value());
    EXPECT_EQ(*intersection, inner);
}

TEST(BufferIntervalTest, Intersect_NoOverlap) {
    BufferInterval a(0, 50);
    BufferInterval b(100, 50);
    auto intersection = a.intersect(b);
    EXPECT_FALSE(intersection.has_value());
}

TEST(BufferIntervalTest, Intersect_Adjacent) {
    BufferInterval a(0, 100);
    BufferInterval b(100, 100);
    auto result = a.intersect(b);
    EXPECT_FALSE(result.has_value());
}

TEST(BufferIntervalTest, Difference_NoOverlap) {
    BufferInterval a(0, 100);
    BufferInterval b(200, 100);
    auto result = a.difference(b);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], a);
}

TEST(BufferIntervalTest, Difference_CompletelyInside) {
    BufferInterval a(0, 100);
    BufferInterval b(25, 50);
    auto result = a.difference(b);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], BufferInterval(0, 25));
    EXPECT_EQ(result[1], BufferInterval(75, 25));
}

TEST(BufferIntervalTest, Difference_OverlapStart) {
    BufferInterval a(100, 100); // 100-200
    BufferInterval b(50, 100);  // 50-150
    auto result = a.difference(b);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], BufferInterval(150, 50));
}

TEST(BufferIntervalTest, Difference_OverlapEnd) {
    BufferInterval a(100, 100); // 100-200
    BufferInterval b(150, 100); // 150-250
    auto result = a.difference(b);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], BufferInterval(100, 50));
}

TEST(BufferIntervalTest, Difference_Contains) {
    BufferInterval a(100, 100); // 100-200
    BufferInterval b(50, 200);  // 50-250
    auto result = a.difference(b);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// ImageInterval Tests
// ============================================================================

TEST(ImageIntervalTest, EmptyInterval) {
    vk::ImageSubresourceRange range;
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = 0;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    ImageInterval empty(range);
    EXPECT_TRUE(empty.empty());
}

TEST(ImageIntervalTest, Contains_CompletelyInside) {
    vk::ImageSubresourceRange outer_range;
    outer_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    outer_range.baseMipLevel = 0;
    outer_range.levelCount = 10;
    outer_range.baseArrayLayer = 0;
    outer_range.layerCount = 10;

    vk::ImageSubresourceRange inner_range;
    inner_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    inner_range.baseMipLevel = 2;
    inner_range.levelCount = 5;
    inner_range.baseArrayLayer = 2;
    inner_range.layerCount = 5;

    ImageInterval outer(outer_range);
    ImageInterval inner(inner_range);

    EXPECT_TRUE(outer.contains(inner));
    EXPECT_FALSE(inner.contains(outer));
}

TEST(ImageIntervalTest, Contains_DifferentAspect) {
    vk::ImageSubresourceRange color_range;
    color_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    color_range.baseMipLevel = 0;
    color_range.levelCount = 10;
    color_range.baseArrayLayer = 0;
    color_range.layerCount = 10;

    vk::ImageSubresourceRange depth_range;
    depth_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth_range.baseMipLevel = 0;
    depth_range.levelCount = 5;
    depth_range.baseArrayLayer = 0;
    depth_range.layerCount = 5;

    ImageInterval color(color_range);
    ImageInterval depth(depth_range);

    EXPECT_FALSE(color.contains(depth));
    EXPECT_FALSE(depth.contains(color));
}

TEST(ImageIntervalTest, Overlaps_PartialOverlap) {
    vk::ImageSubresourceRange a_range;
    a_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    a_range.baseMipLevel = 0;
    a_range.levelCount = 5;
    a_range.baseArrayLayer = 0;
    a_range.layerCount = 5;

    vk::ImageSubresourceRange b_range;
    b_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    b_range.baseMipLevel = 3;
    b_range.levelCount = 5;
    b_range.baseArrayLayer = 3;
    b_range.layerCount = 5;

    ImageInterval a(a_range);
    ImageInterval b(b_range);

    EXPECT_TRUE(a.overlaps(b));
    EXPECT_TRUE(b.overlaps(a));
}

TEST(ImageIntervalTest, Overlaps_DifferentAspect) {
    vk::ImageSubresourceRange color_range;
    color_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    color_range.baseMipLevel = 0;
    color_range.levelCount = 10;
    color_range.baseArrayLayer = 0;
    color_range.layerCount = 10;

    vk::ImageSubresourceRange depth_range;
    depth_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth_range.baseMipLevel = 0;
    depth_range.levelCount = 10;
    depth_range.baseArrayLayer = 0;
    depth_range.layerCount = 10;

    ImageInterval color(color_range);
    ImageInterval depth(depth_range);

    EXPECT_FALSE(color.overlaps(depth));
}

TEST(ImageIntervalTest, Merge_Overlapping) {
    vk::ImageSubresourceRange a_range;
    a_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    a_range.baseMipLevel = 0;
    a_range.levelCount = 5;
    a_range.baseArrayLayer = 0;
    a_range.layerCount = 5;

    vk::ImageSubresourceRange b_range;
    b_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    b_range.baseMipLevel = 3;
    b_range.levelCount = 5;
    b_range.baseArrayLayer = 3;
    b_range.layerCount = 5;

    ImageInterval a(a_range);
    ImageInterval b(b_range);

    auto merged = a.merge(b);
    ASSERT_TRUE(merged.has_value());
    EXPECT_EQ(merged->range.baseMipLevel, 0);
    EXPECT_EQ(merged->range.levelCount, 8);
    EXPECT_EQ(merged->range.baseArrayLayer, 0);
    EXPECT_EQ(merged->range.layerCount, 8);
}

TEST(ImageIntervalTest, Merge_DifferentAspect) {
    vk::ImageSubresourceRange color_range;
    color_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    color_range.baseMipLevel = 0;
    color_range.levelCount = 10;
    color_range.baseArrayLayer = 0;
    color_range.layerCount = 10;

    vk::ImageSubresourceRange depth_range;
    depth_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth_range.baseMipLevel = 0;
    depth_range.levelCount = 10;
    depth_range.baseArrayLayer = 0;
    depth_range.layerCount = 10;

    ImageInterval color(color_range);
    ImageInterval depth(depth_range);

    auto merged = color.merge(depth);
    EXPECT_FALSE(merged.has_value());
}

TEST(ImageIntervalTest, Intersect_Overlapping) {
    vk::ImageSubresourceRange a_range;
    a_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    a_range.baseMipLevel = 0;
    a_range.levelCount = 5;
    a_range.baseArrayLayer = 0;
    a_range.layerCount = 5;

    vk::ImageSubresourceRange b_range;
    b_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    b_range.baseMipLevel = 3;
    b_range.levelCount = 5;
    b_range.baseArrayLayer = 3;
    b_range.layerCount = 5;

    ImageInterval a(a_range);
    ImageInterval b(b_range);

    auto intersection = a.intersect(b);
    ASSERT_TRUE(intersection.has_value());
    EXPECT_EQ(intersection->range.baseMipLevel, 3);
    EXPECT_EQ(intersection->range.levelCount, 2);
    EXPECT_EQ(intersection->range.baseArrayLayer, 3);
    EXPECT_EQ(intersection->range.layerCount, 2);
}

TEST(ImageIntervalTest, Intersect_NoOverlap) {
    vk::ImageSubresourceRange a_range;
    a_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    a_range.baseMipLevel = 0;
    a_range.levelCount = 5;
    a_range.baseArrayLayer = 0;
    a_range.layerCount = 5;

    vk::ImageSubresourceRange b_range;
    b_range.aspectMask = vk::ImageAspectFlagBits::eColor;
    b_range.baseMipLevel = 10;
    b_range.levelCount = 5;
    b_range.baseArrayLayer = 10;
    b_range.layerCount = 5;

    ImageInterval a(a_range);
    ImageInterval b(b_range);

    auto intersection = a.intersect(b);
    EXPECT_FALSE(intersection.has_value());
}
