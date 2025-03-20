#include "VulkanWrapper/Memory/Allocator.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include <vk_mem_alloc.h>

namespace vw {

Allocator::Allocator(VmaAllocator allocator)
    : ObjectWithHandle<VmaAllocator>{allocator} {}

IndexBuffer Allocator::allocate_index_buffer(VkDeviceSize size) {
    return Buffer<unsigned, false, IndexBufferUsage>{allocate_buffer(
        size * sizeof(unsigned), false, vk::BufferUsageFlags(IndexBufferUsage),
        vk::SharingMode::eExclusive)};
}

std::shared_ptr<Image> Allocator::create_image_2D(uint32_t width,
                                                  uint32_t height, bool mipmap,
                                                  vk::Format format,
                                                  vk::ImageUsageFlags usage) {
    VkImageCreateInfo create_info =
        vk::ImageCreateInfo()
            .setExtent(vk::Extent3D(width, height, 1))
            .setMipLevels(1)
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
    return std::make_shared<Image>(vk::Image(image), width, height, format,
                                   usage, this, allocation);
}

Allocator::~Allocator() { vmaDestroyAllocator(handle()); }

BufferBase Allocator::allocate_buffer(VkDeviceSize size, bool host_visible,
                                      vk::BufferUsageFlags usage,
                                      vk::SharingMode sharing_mode) {
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
