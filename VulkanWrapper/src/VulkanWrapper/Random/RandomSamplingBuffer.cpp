#include "VulkanWrapper/Random/RandomSamplingBuffer.h"

#include <random>

namespace vw {

HemisphereSamples generate_hemisphere_samples() {
    std::random_device rd;
    return generate_hemisphere_samples(rd());
}

HemisphereSamples generate_hemisphere_samples(std::uint32_t seed) {
    HemisphereSamples samples{};

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (std::size_t i = 0; i < HEMISPHERE_SAMPLE_COUNT; ++i) {
        samples.samples[i] = glm::vec2(dist(rng), dist(rng));
    }

    return samples;
}

HemisphereSamplesBuffer
create_hemisphere_samples_buffer(const Allocator &allocator) {
    auto buffer =
        create_buffer<HemisphereSamples, true, StorageBufferUsage>(allocator,
                                                                   1);
    buffer.write(generate_hemisphere_samples(), 0);
    return buffer;
}

HemisphereSamplesBuffer
create_hemisphere_samples_buffer(const Allocator &allocator,
                                 std::uint32_t seed) {
    auto buffer =
        create_buffer<HemisphereSamples, true, StorageBufferUsage>(allocator,
                                                                   1);
    buffer.write(generate_hemisphere_samples(seed), 0);
    return buffer;
}

} // namespace vw
