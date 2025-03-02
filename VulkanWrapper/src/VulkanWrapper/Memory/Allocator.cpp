#include "VulkanWrapper/Memory/Allocator.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include <vk_mem_alloc.h>

namespace vw {

Allocator::Allocator(VmaAllocator allocator)
    : ObjectWithHandle<VmaAllocator>{allocator} {}

Allocator::~Allocator() { vmaDestroyAllocator(handle()); }

BufferBase Allocator::allocate_buffer(VkDeviceSize size, bool host_visible,
                                      vk::BufferUsageFlags usage,
                                      vk::SharingMode sharing_mode) {
    VmaAllocationCreateInfo allocation_info{};
    if (host_visible)
        allocation_info.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;

    VkBufferCreateInfo buffer_info =
        vk::BufferCreateInfo().setUsage(usage).setSize(size).setSharingMode(
            sharing_mode);

    VmaAllocation allocation;
    VkBuffer buffer;
    vmaCreateBuffer(handle(), &buffer_info, &allocation_info, &buffer,
                    &allocation, nullptr);
    return BufferBase{*this, buffer, allocation, size};
}

AllocatorBuilder::AllocatorBuilder(const Instance &instance,
                                   const Device &device)
    : m_instance{instance}
    , m_device{device} {}

Allocator AllocatorBuilder::build() && {
    VmaAllocatorCreateInfo info{};
    info.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
    info.device = m_device.handle();
    info.instance = m_instance.handle();
    info.physicalDevice = m_device.physical_device();

    VmaAllocator allocator;
    assert(vk::Result(vmaCreateAllocator(&info, &allocator)) ==
           vk::Result::eSuccess);

    return Allocator{allocator};
}

} // namespace vw
