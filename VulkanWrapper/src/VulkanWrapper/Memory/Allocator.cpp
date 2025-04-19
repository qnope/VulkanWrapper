#include "VulkanWrapper/Memory/Allocator.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include <vk_mem_alloc.h>

namespace vw {
namespace {
MipLevel mip_level_from_size(Width width, Height height, Depth depth) {
    auto size = std::max({uint32_t(width), uint32_t(height), uint32_t(depth)});
    return static_cast<MipLevel>(uint32_t(std::log2(size)) + 1);
}
} // namespace

Allocator::Allocator(VmaAllocator allocator)
    : ObjectWithHandle<VmaAllocator>{allocator} {}

IndexBuffer Allocator::allocate_index_buffer(VkDeviceSize size) const {
    return Buffer<unsigned, false, IndexBufferUsage>{allocate_buffer(
        size * sizeof(unsigned), false, vk::BufferUsageFlags(IndexBufferUsage),
        vk::SharingMode::eExclusive)};
}

std::shared_ptr<const Image>
Allocator::create_image_2D(Width width, Height height, bool mipmap,
                           vk::Format format, vk::ImageUsageFlags usage) const {
    const auto mip_levels = [&] {
        if (mipmap)
            return mip_level_from_size(width, height, Depth(1));
        return MipLevel(1);
    }();

    const VkImageCreateInfo create_info =
        vk::ImageCreateInfo()
            .setExtent(vk::Extent3D(uint32_t(width), uint32_t(height), 1))
            .setMipLevels(uint32_t(mip_levels))
            .setArrayLayers(1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setImageType(vk::ImageType::e2D)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setFormat(format)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setUsage(usage);

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocation allocation = nullptr;
    VkImage image = nullptr;
    vmaCreateImage(handle(), &create_info, &allocation_info, &image,
                   &allocation, nullptr);
    return std::make_shared<const Image>(vk::Image(image), width, height,
                                         Depth(1), mip_levels, format, usage,
                                         this, allocation);
}

Allocator::~Allocator() { vmaDestroyAllocator(handle()); }

BufferBase Allocator::allocate_buffer(VkDeviceSize size, bool host_visible,
                                      vk::BufferUsageFlags usage,
                                      vk::SharingMode sharing_mode) const {
    VmaAllocationCreateInfo allocation_info{};
    if (host_visible) {
        allocation_info.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBufferCreateInfo buffer_info =
        vk::BufferCreateInfo().setUsage(usage).setSize(size).setSharingMode(
            sharing_mode);

    VmaAllocation allocation = nullptr;
    VkBuffer buffer = nullptr;
    vmaCreateBuffer(handle(), &buffer_info, &allocation_info, &buffer,
                    &allocation, nullptr);
    return BufferBase{*this, buffer, allocation, size};
}

AllocatorBuilder::AllocatorBuilder(const Instance &instance,
                                   const Device &device)
    : m_instance{&instance}
    , m_device{&device} {}

Allocator AllocatorBuilder::build() && {
    VmaAllocatorCreateInfo info{};
    info.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
    info.device = m_device->handle();
    info.instance = m_instance->handle();
    info.physicalDevice = m_device->physical_device();

    VmaAllocator allocator = nullptr;
    assert(vk::Result(vmaCreateAllocator(&info, &allocator)) ==
           vk::Result::eSuccess);

    return Allocator{allocator};
}

} // namespace vw
