#include "VulkanWrapper/Memory/Interval.h"

namespace vw {

// ============================================================================
// BufferInterval Implementation
// ============================================================================

bool BufferInterval::contains(const BufferInterval &other) const {
    if (empty() || other.empty()) {
        return false;
    }
    return offset <= other.offset && end() >= other.end();
}

bool BufferInterval::overlaps(const BufferInterval &other) const {
    if (empty() || other.empty()) {
        return false;
    }
    return offset < other.end() && other.offset < end();
}

std::optional<BufferInterval>
BufferInterval::merge(const BufferInterval &other) const {
    if (empty()) {
        return other.empty() ? std::nullopt : std::optional{other};
    }
    if (other.empty()) {
        return *this;
    }

    // Check if intervals overlap or are adjacent
    if (!overlaps(other) && end() != other.offset && other.end() != offset) {
        return std::nullopt;
    }

    vk::DeviceSize new_offset = std::min(offset, other.offset);
    vk::DeviceSize new_end = std::max(end(), other.end());
    return BufferInterval{new_offset, new_end - new_offset};
}

std::optional<BufferInterval>
BufferInterval::intersect(const BufferInterval &other) const {
    if (!overlaps(other)) {
        return std::nullopt;
    }

    vk::DeviceSize new_offset = std::max(offset, other.offset);
    vk::DeviceSize new_end = std::min(end(), other.end());
    return BufferInterval{new_offset, new_end - new_offset};
}

// ============================================================================
// ImageInterval Implementation
// ============================================================================

bool ImageInterval::contains(const ImageInterval &other) const {
    if (empty() || other.empty()) {
        return false;
    }

    // Aspect masks must match exactly for containment
    if (range.aspectMask != other.range.aspectMask) {
        return false;
    }

    // Check mip level range
    uint32_t this_mip_end = range.baseMipLevel + range.levelCount;
    uint32_t other_mip_end = other.range.baseMipLevel + other.range.levelCount;
    bool mips_contained = range.baseMipLevel <= other.range.baseMipLevel &&
                          this_mip_end >= other_mip_end;

    // Check array layer range
    uint32_t this_layer_end = range.baseArrayLayer + range.layerCount;
    uint32_t other_layer_end =
        other.range.baseArrayLayer + other.range.layerCount;
    bool layers_contained =
        range.baseArrayLayer <= other.range.baseArrayLayer &&
        this_layer_end >= other_layer_end;

    return mips_contained && layers_contained;
}

bool ImageInterval::overlaps(const ImageInterval &other) const {
    if (empty() || other.empty()) {
        return false;
    }

    // Check if aspect masks intersect
    if ((range.aspectMask & other.range.aspectMask) ==
        vk::ImageAspectFlags{}) {
        return false;
    }

    // Check mip level overlap
    uint32_t this_mip_end = range.baseMipLevel + range.levelCount;
    uint32_t other_mip_end = other.range.baseMipLevel + other.range.levelCount;
    bool mips_overlap = range.baseMipLevel < other_mip_end &&
                        other.range.baseMipLevel < this_mip_end;

    // Check array layer overlap
    uint32_t this_layer_end = range.baseArrayLayer + range.layerCount;
    uint32_t other_layer_end =
        other.range.baseArrayLayer + other.range.layerCount;
    bool layers_overlap = range.baseArrayLayer < other_layer_end &&
                          other.range.baseArrayLayer < this_layer_end;

    return mips_overlap && layers_overlap;
}

std::optional<ImageInterval>
ImageInterval::merge(const ImageInterval &other) const {
    if (empty()) {
        return other.empty() ? std::nullopt : std::optional{other};
    }
    if (other.empty()) {
        return *this;
    }

    // Aspect masks must match for merging
    if (range.aspectMask != other.range.aspectMask) {
        return std::nullopt;
    }

    // Check if intervals overlap or are adjacent in both dimensions
    uint32_t this_mip_end = range.baseMipLevel + range.levelCount;
    uint32_t other_mip_end = other.range.baseMipLevel + other.range.levelCount;
    bool mips_overlap_or_adjacent =
        range.baseMipLevel <= other_mip_end &&
        other.range.baseMipLevel <= this_mip_end;

    uint32_t this_layer_end = range.baseArrayLayer + range.layerCount;
    uint32_t other_layer_end =
        other.range.baseArrayLayer + other.range.layerCount;
    bool layers_overlap_or_adjacent =
        range.baseArrayLayer <= other_layer_end &&
        other.range.baseArrayLayer <= this_layer_end;

    if (!mips_overlap_or_adjacent || !layers_overlap_or_adjacent) {
        return std::nullopt;
    }

    vk::ImageSubresourceRange merged;
    merged.aspectMask = range.aspectMask;
    merged.baseMipLevel = std::min(range.baseMipLevel, other.range.baseMipLevel);
    merged.levelCount = std::max(this_mip_end, other_mip_end) - merged.baseMipLevel;
    merged.baseArrayLayer =
        std::min(range.baseArrayLayer, other.range.baseArrayLayer);
    merged.layerCount =
        std::max(this_layer_end, other_layer_end) - merged.baseArrayLayer;

    return ImageInterval{merged};
}

std::optional<ImageInterval>
ImageInterval::intersect(const ImageInterval &other) const {
    if (!overlaps(other)) {
        return std::nullopt;
    }

    vk::ImageSubresourceRange intersection;
    intersection.aspectMask = range.aspectMask & other.range.aspectMask;

    uint32_t this_mip_end = range.baseMipLevel + range.levelCount;
    uint32_t other_mip_end = other.range.baseMipLevel + other.range.levelCount;
    intersection.baseMipLevel =
        std::max(range.baseMipLevel, other.range.baseMipLevel);
    intersection.levelCount =
        std::min(this_mip_end, other_mip_end) - intersection.baseMipLevel;

    uint32_t this_layer_end = range.baseArrayLayer + range.layerCount;
    uint32_t other_layer_end =
        other.range.baseArrayLayer + other.range.layerCount;
    intersection.baseArrayLayer =
        std::max(range.baseArrayLayer, other.range.baseArrayLayer);
    intersection.layerCount =
        std::min(this_layer_end, other_layer_end) - intersection.baseArrayLayer;

    return ImageInterval{intersection};
}

} // namespace vw
