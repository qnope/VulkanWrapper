#pragma once
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {

template <typename T, bool HostVisible>
Buffer<T, HostVisible, VertexBufferUsage>
Allocator::allocate_vertex_buffer(VkDeviceSize size) const {
    return Buffer<T, HostVisible, VertexBufferUsage>{
        allocate_buffer(size * sizeof(T), HostVisible,
                        vk::BufferUsageFlags(VertexBufferUsage),
                        vk::SharingMode::eExclusive)};
}

template <typename T, bool HostVisible, VkBufferUsageFlags2 Usage>
Buffer<T, HostVisible, Usage>
Allocator::create_buffer(vk::DeviceSize number_elements) const {
    return {allocate_buffer(number_elements * sizeof(T), HostVisible,
                            vk::BufferUsageFlags(Usage),
                            vk::SharingMode::eExclusive)};
}

template <typename BufferType>
BufferType
Allocator::create_buffer(vk::DeviceSize number_elements) const {
    return create_buffer<typename BufferType::type,
                         BufferType::host_visible,
                         VkBufferUsageFlags(BufferType::flags)>(
        number_elements);
}

} // namespace vw
