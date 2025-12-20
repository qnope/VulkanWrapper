#pragma once

#include "VulkanWrapper/3rd_party.h"
#include <optional>

namespace vw {

/**
 * Represents a contiguous range in a buffer defined by offset and size.
 */
struct BufferInterval {
    vk::DeviceSize offset;
    vk::DeviceSize size;

    BufferInterval()
        : offset(0)
        , size(0) {}
    BufferInterval(vk::DeviceSize offset, vk::DeviceSize size)
        : offset(offset)
        , size(size) {}

    /**
     * Returns the end offset (exclusive) of this interval.
     */
    [[nodiscard]] vk::DeviceSize end() const { return offset + size; }

    /**
     * Checks if this interval is empty (size == 0).
     */
    [[nodiscard]] bool empty() const { return size == 0; }

    /**
     * Checks if this interval completely contains another interval.
     */
    [[nodiscard]] bool contains(const BufferInterval &other) const;

    /**
     * Checks if this interval overlaps with another interval.
     */
    [[nodiscard]] bool overlaps(const BufferInterval &other) const;

    /**
     * Merges this interval with another, returning the smallest interval
     * that contains both. Returns nullopt if intervals don't overlap or touch.
     */
    [[nodiscard]] std::optional<BufferInterval>
    merge(const BufferInterval &other) const;

    /**
     * Returns the intersection of this interval with another.
     * Returns nullopt if intervals don't overlap.
     */
    [[nodiscard]] std::optional<BufferInterval>
    intersect(const BufferInterval &other) const;

    /**
     * Returns the difference of this interval and another (this - other).
     * Returns a list of intervals that represent the remaining area.
     */
    [[nodiscard]] std::vector<BufferInterval>
    difference(const BufferInterval &other) const;

    bool operator==(const BufferInterval &other) const {
        return offset == other.offset && size == other.size;
    }

    bool operator!=(const BufferInterval &other) const {
        return !(*this == other);
    }
};

/**
 * Represents a subresource range in an image.
 */
struct ImageInterval {
    vk::ImageSubresourceRange range;

    ImageInterval()
        : range() {}
    explicit ImageInterval(const vk::ImageSubresourceRange &range)
        : range(range) {}

    /**
     * Checks if this interval is empty (layerCount or levelCount == 0).
     */
    [[nodiscard]] bool empty() const {
        return range.layerCount == 0 || range.levelCount == 0;
    }

    /**
     * Checks if this interval completely contains another interval.
     * Both must have the same aspect mask.
     */
    [[nodiscard]] bool contains(const ImageInterval &other) const;

    /**
     * Checks if this interval overlaps with another interval.
     * Only overlaps if aspect masks intersect.
     */
    [[nodiscard]] bool overlaps(const ImageInterval &other) const;

    /**
     * Merges this interval with another, returning the smallest interval
     * that contains both. Returns nullopt if intervals don't overlap or touch,
     * or if aspect masks don't match.
     */
    [[nodiscard]] std::optional<ImageInterval>
    merge(const ImageInterval &other) const;

    /**
     * Returns the intersection of this interval with another.
     * Returns nullopt if intervals don't overlap.
     */
    [[nodiscard]] std::optional<ImageInterval>
    intersect(const ImageInterval &other) const;

    /**
     * Returns the difference of this interval and another (this - other).
     * Returns a list of intervals that represent the remaining area.
     */
    [[nodiscard]] std::vector<ImageInterval>
    difference(const ImageInterval &other) const;

    bool operator==(const ImageInterval &other) const {
        return range.aspectMask == other.range.aspectMask &&
               range.baseMipLevel == other.range.baseMipLevel &&
               range.levelCount == other.range.levelCount &&
               range.baseArrayLayer == other.range.baseArrayLayer &&
               range.layerCount == other.range.layerCount;
    }

    bool operator!=(const ImageInterval &other) const {
        return !(*this == other);
    }
};

} // namespace vw
