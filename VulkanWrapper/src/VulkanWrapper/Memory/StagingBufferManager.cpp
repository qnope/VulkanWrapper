#include "VulkanWrapper/Memory/StagingBufferManager.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageLoader.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Mipmap.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Memory/Barrier.h"

constexpr auto STAGING_BUFFER_SIZE = 1 << 22;

namespace vw {

static vk::DeviceSize compute_size(vk::DeviceSize size) {
    const auto max_size =
        double(std::max<vk::DeviceSize>(size, STAGING_BUFFER_SIZE));

    return vk::DeviceSize(std::pow(2, std::ceil(std::log2(max_size))));
}

static CommandPool create_command_pool(std::shared_ptr<const Device> device) {
    return CommandPoolBuilder(device).build();
}

StagingBufferManager::StagingBufferManager(std::shared_ptr<const Device> device,
                                           std::shared_ptr<const Allocator> allocator)
    : m_device{device}
    , m_allocator{allocator}
    , m_command_pool(create_command_pool(device))
    , m_staging_buffers{allocator}
    , m_sampler(SamplerBuilder{device}.build()) {}

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

CombinedImage
StagingBufferManager::stage_image_from_path(const std::filesystem::path &path,
                                            bool mipmaps) {
    const auto img_description = load_image(path);
    auto usage =
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

    if (mipmaps)
        usage |= vk::ImageUsageFlagBits::eTransferSrc;

    auto image = m_allocator->create_image_2D(img_description.width,
                                              img_description.height, mipmaps,
                                              vk::Format::eR8G8B8A8Srgb, usage);

    auto [staging_buffer, offset] =
        m_staging_buffers.create_buffer(img_description.pixels.size());

    auto function = [buffer = staging_buffer->handle(), offset, image,
                     width = img_description.width,
                     height = img_description.height,
                     mipmaps](vk::CommandBuffer cmd_buffer) {
        const auto region =
            vk::BufferImageCopy()
                .setBufferOffset(offset)
                .setImageExtent(image->extent3D())
                .setImageSubresource(image->mip_level_layer(MipLevel(0)));

        execute_image_barrier_undefined_to_transfer_dst(cmd_buffer, image);

        cmd_buffer.copyBufferToImage(buffer, image->handle(),
                                     vk::ImageLayout::eTransferDstOptimal,
                                     region);

        if (mipmaps) {
            execute_image_barrier_transfer_dst_to_src(cmd_buffer, image,
                                                      MipLevel(0));
            generate_mipmap(cmd_buffer, image);
            execute_image_barrier_transfer_src_to_dst(cmd_buffer, image);
        } else {
            execute_image_barrier_transfer_dst_to_sampled(cmd_buffer, image);
        }
    };

    m_transfer_functions.emplace_back(function);
    staging_buffer->copy(img_description.pixels, offset);

    auto image_view = ImageViewBuilder(m_device, image)
                          .setImageType(vk::ImageViewType::e2D)
                          .build();

    return {std::move(image_view), m_sampler};
}

} // namespace vw
