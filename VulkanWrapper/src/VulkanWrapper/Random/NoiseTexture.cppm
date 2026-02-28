export module vw.random:noise_texture;
import std3rd;
import vulkan3rd;
import vw.vulkan;
import vw.memory;

export namespace vw {

/// Size of the noise texture (width and height)
constexpr std::uint32_t NOISE_TEXTURE_SIZE = 4096;

/**
 * @brief Creates a noise texture for random sampling decorrelation
 *
 * This class manages a 4096x4096 RG32F texture filled with random values
 * in [0, 1). The texture is used to decorrelate neighboring pixels when
 * sampling from the hemisphere samples buffer via Cranley-Patterson rotation.
 */
class NoiseTexture {
  public:
    /**
     * @brief Construct a NoiseTexture
     */
    NoiseTexture(std::shared_ptr<Device> device,
                 std::shared_ptr<Allocator> allocator, Queue &queue);

    /**
     * @brief Construct a NoiseTexture with specific seed
     */
    NoiseTexture(std::shared_ptr<Device> device,
                 std::shared_ptr<Allocator> allocator, Queue &queue,
                 std::uint32_t seed);

    /// Get the image handle
    [[nodiscard]] const Image &image() const;

    /// Get the image view handle
    [[nodiscard]] const ImageView &view() const;

    /// Get the sampler handle
    [[nodiscard]] const Sampler &sampler() const;

    /// Get as CombinedImage for descriptor binding
    [[nodiscard]] CombinedImage combined_image() const;

  private:
    void initialize(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator, Queue &queue,
                    std::uint32_t seed);

    std::shared_ptr<const Image> m_image;
    std::shared_ptr<const ImageView> m_view;
    std::shared_ptr<const Sampler> m_sampler;
};

} // namespace vw
