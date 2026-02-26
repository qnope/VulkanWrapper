module;
#include "VulkanWrapper/3rd_party.h"
#include <array>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

export module vw:random;
import :core;
import :utils;
import :vulkan;
import :memory;
import :image;

export namespace vw {

constexpr std::uint32_t NOISE_TEXTURE_SIZE = 4096;

class NoiseTexture {
  public:
    NoiseTexture(std::shared_ptr<Device> device,
                 std::shared_ptr<Allocator> allocator, Queue &queue);

    NoiseTexture(std::shared_ptr<Device> device,
                 std::shared_ptr<Allocator> allocator, Queue &queue,
                 std::uint32_t seed);

    [[nodiscard]] const Image &image() const;
    [[nodiscard]] const ImageView &view() const;
    [[nodiscard]] const Sampler &sampler() const;
    [[nodiscard]] CombinedImage combined_image() const;

  private:
    void initialize(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator, Queue &queue,
                    std::uint32_t seed);

    std::shared_ptr<const Image> m_image;
    std::shared_ptr<const ImageView> m_view;
    std::shared_ptr<const Sampler> m_sampler;
};

constexpr std::size_t DUAL_SAMPLE_COUNT = 4096;

struct DualRandomSample {
    std::array<glm::vec2, DUAL_SAMPLE_COUNT> samples;
};

using DualRandomSampleBuffer =
    Buffer<DualRandomSample, true, StorageBufferUsage>;

[[nodiscard]] DualRandomSample generate_hemisphere_samples();
[[nodiscard]] DualRandomSample generate_hemisphere_samples(std::uint32_t seed);

[[nodiscard]] DualRandomSampleBuffer
create_hemisphere_samples_buffer(const Allocator &allocator);

[[nodiscard]] DualRandomSampleBuffer
create_hemisphere_samples_buffer(const Allocator &allocator,
                                 std::uint32_t seed);

} // namespace vw
