export module vw.memory:allocate_buffer_utils;
import vulkan3rd;
import vma3rd;
import vw.utils;
import :allocator;
import :buffer;

export namespace vw {

inline BufferBase allocate_buffer(const Allocator &allocator, VkDeviceSize size,
                                  bool host_visible, vk::BufferUsageFlags usage,
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
    vmaCreateBufferWithAlignment(allocator.handle(), &buffer_info,
                                 &allocation_info, DefaultBufferAlignment,
                                 &buffer, &allocation, nullptr);

    return BufferBase{allocator.device(), allocator.shared_from_this(), buffer,
                      allocation, size};
}

inline IndexBuffer allocate_index_buffer(const Allocator &allocator,
                                         VkDeviceSize size) {
    return IndexBuffer{allocate_buffer(
        allocator, size * sizeof(unsigned), false,
        vk::BufferUsageFlags(IndexBufferUsage), vk::SharingMode::eExclusive)};
}

template <typename T, bool HostVisible>
Buffer<T, HostVisible, VertexBufferUsage>
allocate_vertex_buffer(const Allocator &allocator, vk::DeviceSize size) {
    return Buffer<T, HostVisible, VertexBufferUsage>{allocate_buffer(
        allocator, size * sizeof(T), HostVisible,
        vk::BufferUsageFlags(VertexBufferUsage), vk::SharingMode::eExclusive)};
}

template <typename T, bool HostVisible, VkBufferUsageFlags2 Usage>
Buffer<T, HostVisible, Usage> create_buffer(const Allocator &allocator,
                                            vk::DeviceSize number_elements) {
    return {allocate_buffer(allocator, number_elements * sizeof(T), HostVisible,
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
