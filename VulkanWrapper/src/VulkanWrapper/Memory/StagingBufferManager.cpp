#include "VulkanWrapper/Memory/StagingBufferManager.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/ImageLoader.h"
#include "VulkanWrapper/Memory/Barrier.h"

constexpr auto STAGING_BUFFER_SIZE = 1 << 22;

namespace vw {

static vk::DeviceSize compute_size(vk::DeviceSize size) {
    const auto max_size =
        double(std::max<vk::DeviceSize>(size, STAGING_BUFFER_SIZE));

    return vk::DeviceSize(std::pow(2, std::ceil(std::log2(max_size))));
}

static CommandPool create_command_pool(Device &device) {
    return CommandPoolBuilder(device).build();
}

StagingBuffer::StagingBuffer(Allocator &allocator, vk::DeviceSize size)
    : m_buffer{
          allocator.create_buffer<std::byte, true,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT>(size)} {}

vk::Buffer StagingBuffer::handle() const noexcept { return m_buffer.handle(); }

vk::DeviceSize StagingBuffer::offset() const noexcept { return m_offset; }

bool StagingBuffer::handle_data(vk::DeviceSize size) const noexcept {
    return (m_buffer.size_bytes() - m_offset) >= size;
}

StagingBufferManager::StagingBufferManager(Device &device, Allocator &allocator)
    : m_device{&device}
    , m_allocator{&allocator}
    , m_command_pool(create_command_pool(device)) {}

vk::CommandBuffer StagingBufferManager::fill_command_buffer() {
    auto cmd_buffer = m_command_pool.allocate(1)[0];

    vk::CommandBufferBeginInfo info(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    std::ignore = cmd_buffer.begin(&info);
    for (auto &function : m_transfer_functions)
        function(cmd_buffer);
    std::ignore = cmd_buffer.end();

    return cmd_buffer;
}

StagingBuffer &StagingBufferManager::get_staging_buffer(vk::DeviceSize size) {
    auto does_buffer_handle_data = [size](const StagingBuffer &buffer) {
        return buffer.handle_data(size);
    };

    auto it = std::ranges::find_if(m_staging_buffers, does_buffer_handle_data);
    if (it != m_staging_buffers.end())
        return *it;

    const auto new_size = compute_size(size);
    return m_staging_buffers.emplace_back(*m_allocator, new_size);
}

std::shared_ptr<Image>
StagingBufferManager::stage_image_from_path(const std::filesystem::path &path) {
    const auto img_description = load_image(path);
    constexpr auto usage =
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

    auto image = m_allocator->create_image_2D(img_description.width,
                                              img_description.height, false,
                                              vk::Format::eR8G8B8A8Srgb, usage);

    auto &staging_buffer = get_staging_buffer(img_description.pixels.size());

    auto function = [buffer = staging_buffer.handle(),
                     offset = staging_buffer.offset(), image = image->handle(),
                     width = img_description.width,
                     height =
                         img_description.height](vk::CommandBuffer cmd_buffer) {
        const auto img_subresource =
            vk::ImageSubresourceLayers()
                .setLayerCount(1)
                .setMipLevel(0)
                .setBaseArrayLayer(0)
                .setAspectMask(vk::ImageAspectFlagBits::eColor);
        const auto region = vk::BufferImageCopy()
                                .setBufferOffset(offset)
                                .setImageExtent(vk::Extent3D(width, height, 1))
                                .setImageSubresource(img_subresource);

        const auto range = vk::ImageSubresourceRange()
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setLayerCount(1)
                               .setLevelCount(1)
                               .setBaseArrayLayer(0)
                               .setBaseMipLevel(0);
        executeImageMemoryBarrierUndefinedToTransferDst(cmd_buffer, image,
                                                        range);
        cmd_buffer.copyBufferToImage(
            buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

        executeImageMemoryBarrierTransferToSampled(cmd_buffer, image, range);
    };

    m_transfer_functions.emplace_back(function);
    staging_buffer.fill_buffer<std::byte>(img_description.pixels);

    return image;
}

} // namespace vw
