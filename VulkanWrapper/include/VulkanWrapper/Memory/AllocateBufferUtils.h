#pragma once
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Memory/Buffer.h"

namespace vw {

template <typename T, bool HostVisible>
Buffer<T, HostVisible, VertexBufferUsage>
allocate_vertex_buffer(const Allocator &allocator, vk::DeviceSize size) {
    return Buffer<T, HostVisible, VertexBufferUsage>{allocator.allocate_buffer(
        size * sizeof(T), HostVisible, vk::BufferUsageFlags(VertexBufferUsage),
        vk::SharingMode::eExclusive)};
}

template <typename T, bool HostVisible, VkBufferUsageFlags2 Usage>
Buffer<T, HostVisible, Usage> create_buffer(const Allocator &allocator,
                                            vk::DeviceSize number_elements) {
    return {allocator.allocate_buffer(number_elements * sizeof(T), HostVisible,
                                      vk::BufferUsageFlags(Usage),
                                      vk::SharingMode::eExclusive)};
}

template <typename BufferType>
BufferType create_buffer(const Allocator &allocator,
                         vk::DeviceSize number_elements) {
    return create_buffer<typename BufferType::type, BufferType::host_visible,
                         VkBufferUsageFlags(BufferType::flags)>(
        allocator, number_elements);
}

} // namespace vw
