export module vw.memory:uniform_buffer_allocator;
import std3rd;
import vulkan3rd;
import :buffer;
import :allocator;
import vw.utils;

export namespace vw {

template <typename T> struct UniformBufferChunk {
    vk::Buffer handle{};
    vk::DeviceSize offset{};
    vk::DeviceSize size{};
    uint32_t index{};

    std::shared_ptr<Buffer<std::byte, true, UniformBufferUsage>> buffer_ref{};

    vk::DescriptorBufferInfo descriptorInfo() const {
        return vk::DescriptorBufferInfo(handle, offset, size);
    }

    void write(const T &value) {
        if (!buffer_ref) {
            throw LogicException::null_pointer("buffer reference");
        }
        BufferBase *base_ptr = static_cast<BufferBase *>(buffer_ref.get());
        base_ptr->write_bytes(&value, sizeof(T), offset);
    }

    void write(std::span<const T> data) {
        if (!buffer_ref) {
            throw LogicException::null_pointer("buffer reference");
        }
        static_cast<BufferBase *>(buffer_ref.get())
            ->write_bytes(data.data(), data.size_bytes(), offset);
    }
};

class UniformBufferAllocator {
  public:
    UniformBufferAllocator(std::shared_ptr<const Allocator> allocator,
                           vk::DeviceSize totalSize,
                           vk::DeviceSize minAlignment = 256);

    template <typename T>
    std::optional<UniformBufferChunk<T>> allocate(size_t count = 1);

    void deallocate(uint32_t index);

    [[nodiscard]] vk::Buffer buffer() const { return m_buffer->handle(); }

    [[nodiscard]] vk::DeviceSize totalSize() const { return m_totalSize; }

    [[nodiscard]] vk::DeviceSize freeSpace() const;

    [[nodiscard]] size_t allocationCount() const;

    void clear();

    [[nodiscard]] std::shared_ptr<Buffer<std::byte, true, UniformBufferUsage>>
    buffer_ref() const {
        return m_buffer;
    }

  private:
    struct Allocation {
        vk::DeviceSize offset{};
        vk::DeviceSize size{};
        bool free{};
    };

    std::shared_ptr<Buffer<std::byte, true, UniformBufferUsage>> m_buffer;
    vk::DeviceSize m_totalSize;
    vk::DeviceSize m_minAlignment;
    std::vector<Allocation> m_allocations;
    uint32_t m_nextIndex;

    vk::DeviceSize align(vk::DeviceSize size) const;
    std::optional<uint32_t> findFreeBlock(vk::DeviceSize size);
};

// Template implementation
template <typename T>
std::optional<UniformBufferChunk<T>>
UniformBufferAllocator::allocate(size_t count) {
    vk::DeviceSize requestedSize = sizeof(T) * count;
    vk::DeviceSize alignedSize = align(requestedSize);

    auto freeBlockIndex = findFreeBlock(alignedSize);
    if (!freeBlockIndex.has_value()) {
        return std::nullopt;
    }

    uint32_t index = *freeBlockIndex;
    // Get values before modifying vector (push_back may reallocate)
    vk::DeviceSize chunkOffset = m_allocations[index].offset;

    // If the free block is larger than needed, split it
    if (m_allocations[index].size > alignedSize) {
        vk::DeviceSize remainingSize = m_allocations[index].size - alignedSize;
        vk::DeviceSize newOffset = chunkOffset + alignedSize;

        // Update current allocation
        m_allocations[index].size = alignedSize;
        m_allocations[index].free = false;

        // Create new free allocation for the remainder
        m_allocations.push_back(
            {.offset = newOffset, .size = remainingSize, .free = true});
    } else {
        m_allocations[index].free = false;
    }

    return UniformBufferChunk<T>{.handle = m_buffer->handle(),
                                 .offset = chunkOffset,
                                 .size = alignedSize,
                                 .index = index,
                                 .buffer_ref = m_buffer};
}

} // namespace vw
