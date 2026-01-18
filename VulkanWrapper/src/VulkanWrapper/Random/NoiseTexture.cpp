#include "VulkanWrapper/Random/NoiseTexture.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Transfer.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Vulkan/Device.h"

#include <random>
#include <vector>

namespace vw {

NoiseTexture::NoiseTexture(std::shared_ptr<Device> device,
                           std::shared_ptr<Allocator> allocator,
                           Queue &queue) {
    std::random_device rd;
    initialize(std::move(device), std::move(allocator), queue, rd());
}

NoiseTexture::NoiseTexture(std::shared_ptr<Device> device,
                           std::shared_ptr<Allocator> allocator, Queue &queue,
                           std::uint32_t seed) {
    initialize(std::move(device), std::move(allocator), queue, seed);
}

void NoiseTexture::initialize(std::shared_ptr<Device> device,
                              std::shared_ptr<Allocator> allocator,
                              Queue &queue, std::uint32_t seed) {
    // Create the image
    m_image = allocator->create_image_2D(
        Width{NOISE_TEXTURE_SIZE}, Height{NOISE_TEXTURE_SIZE},
        false, // no mipmaps
        vk::Format::eR32G32Sfloat,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

    // Create image view
    m_view = ImageViewBuilder(device, m_image).build();

    // Create sampler with repeat wrapping (default)
    m_sampler = SamplerBuilder(device).build();

    // Generate random data
    const std::size_t pixel_count =
        static_cast<std::size_t>(NOISE_TEXTURE_SIZE) * NOISE_TEXTURE_SIZE;
    std::vector<glm::vec2> noise_data(pixel_count);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (std::size_t i = 0; i < pixel_count; ++i) {
        noise_data[i] = glm::vec2(dist(rng), dist(rng));
    }

    // Create staging buffer and upload data
    auto staging_buffer = create_buffer<glm::vec2, true, StagingBufferUsage>(
        *allocator, pixel_count);
    staging_buffer.write(std::span<const glm::vec2>(noise_data), 0);

    // Record transfer commands
    auto cmd_pool = CommandPoolBuilder(device).build();
    auto cmd = cmd_pool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Use Transfer to handle the copy with automatic barrier management
    Transfer transfer;
    transfer.copyBufferToImage(cmd, staging_buffer.handle(), m_image, 0);

    // Transition to shader read optimal for sampling
    // Use AllCommands stage to be generic - actual usage determines final stage
    auto &tracker = transfer.resourceTracker();
    tracker.request(Barrier::ImageState{
        .image = m_image->handle(),
        .subresourceRange = m_image->full_range(),
        .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .stage = vk::PipelineStageFlagBits2::eAllCommands,
        .access = vk::AccessFlagBits2::eShaderRead});
    tracker.flush(cmd);

    std::ignore = cmd.end();

    // Submit and wait
    queue.enqueue_command_buffer(cmd);
    queue.submit({}, {}, {}).wait();
}

const Image &NoiseTexture::image() const { return *m_image; }

const ImageView &NoiseTexture::view() const { return *m_view; }

const Sampler &NoiseTexture::sampler() const { return *m_sampler; }

CombinedImage NoiseTexture::combined_image() const {
    return CombinedImage(m_view, m_sampler);
}

} // namespace vw
