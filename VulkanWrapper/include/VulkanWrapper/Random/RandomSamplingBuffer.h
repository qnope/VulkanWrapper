#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferUsage.h"

#include <array>
#include <cstdint>

namespace vw {

/// Number of precomputed hemisphere samples
constexpr std::size_t DUAL_SAMPLE_COUNT = 4096;

/// Structure containing all hemisphere samples (matches GLSL layout)
/// Each sample is a vec2 with random values in [0, 1) for use with
/// cosine-weighted hemisphere sampling
struct DualRandomSample {
    std::array<glm::vec2, DUAL_SAMPLE_COUNT> samples;
};

/// Buffer type alias for hemisphere samples storage buffer
using DualRandomSampleBuffer =
    Buffer<DualRandomSample, true, StorageBufferUsage>;

/// Generate hemisphere samples using std::mt19937
/// @return DualRandomSample structure with values in [0, 1)
[[nodiscard]] DualRandomSample generate_hemisphere_samples();

/// Generate hemisphere samples using a specific seed for reproducibility
/// @param seed Random seed
/// @return DualRandomSample structure with values in [0, 1)
[[nodiscard]] DualRandomSample generate_hemisphere_samples(std::uint32_t seed);

/// Create a storage buffer filled with hemisphere samples
/// @param allocator Memory allocator
/// @return Host-visible storage buffer containing hemisphere samples
[[nodiscard]] DualRandomSampleBuffer
create_hemisphere_samples_buffer(const Allocator &allocator);

/// Create a storage buffer filled with hemisphere samples using specific seed
/// @param allocator Memory allocator
/// @param seed Random seed for reproducibility
/// @return Host-visible storage buffer containing hemisphere samples
[[nodiscard]] DualRandomSampleBuffer
create_hemisphere_samples_buffer(const Allocator &allocator,
                                 std::uint32_t seed);

} // namespace vw
