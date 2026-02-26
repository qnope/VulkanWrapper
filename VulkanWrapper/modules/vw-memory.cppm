module;
#include "VulkanWrapper/3rd_party.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

export module vw:memory;
import :core;
import :utils;
import :vulkan;
import :command;
import :synchronization;
import :image;

export namespace vw {

// ---- BufferBase ----

class BufferBase : public ObjectWithHandle<vk::Buffer> {
  public:
    BufferBase(std::shared_ptr<const Device> device,
               std::shared_ptr<const Allocator> allocator, vk::Buffer buffer,
               VmaAllocation allocation, VkDeviceSize size);

    BufferBase(const BufferBase &) = delete;
    BufferBase &operator=(const BufferBase &) = delete;

    BufferBase(BufferBase &&other) noexcept = default;
    BufferBase &operator=(BufferBase &&other) noexcept = default;

    [[nodiscard]] VkDeviceSize size_bytes() const noexcept;
    [[nodiscard]] vk::DeviceAddress device_address() const;

    void write_bytes(const void *data, VkDeviceSize size, VkDeviceSize offset);
    [[nodiscard]] std::vector<std::byte> read_bytes(VkDeviceSize offset,
                                                    VkDeviceSize size) const;

    ~BufferBase();

  private:
    struct Data {
        std::shared_ptr<const Device> m_device;
        std::shared_ptr<const Allocator> m_allocator;
        VmaAllocation m_allocation;
        VkDeviceSize m_size_in_bytes;
    };
    std::unique_ptr<Data> m_data;
};

// ---- Buffer<T, HostVisible, Flags> ----

template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
  public:
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);

    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (vk::BufferUsageFlags(flags) & usage) == usage;
    }

    Buffer(BufferBase &&bufferBase)
        : BufferBase(std::move(bufferBase)) {}

    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::write_bytes(data.data(), data.size_bytes(),
                                offset * sizeof(T));
    }

    void write(const T &element, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::write_bytes(&element, sizeof(T), offset * sizeof(T));
    }

    template <typename U>
    void write_bytes(std::span<const U> data, std::size_t offset) {
        BufferBase::write_bytes(data.data(), data.size_bytes(),
                                offset * sizeof(T));
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_bytes() / sizeof(T);
    }

    [[nodiscard]] std::vector<T> read_as_vector(std::size_t offset,
                                                std::size_t count) const
        requires(HostVisible)
    {
        auto bytes =
            BufferBase::read_bytes(offset * sizeof(T), count * sizeof(T));
        std::vector<T> result(count);
        std::memcpy(result.data(), bytes.data(), bytes.size());
        return result;
    }
};

using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

// ---- Allocator ----

class Allocator : public std::enable_shared_from_this<Allocator> {
    friend class AllocatorBuilder;

  public:
    Allocator(const Allocator &) = delete;
    Allocator(Allocator &&) = delete;

    Allocator &operator=(Allocator &&) = delete;
    Allocator &operator=(const Allocator &) = delete;

    [[nodiscard]] VmaAllocator handle() const noexcept;

    [[nodiscard]] IndexBuffer allocate_index_buffer(VkDeviceSize size) const;

    [[nodiscard]] std::shared_ptr<const Image>
    create_image_2D(Width width, Height height, bool mipmap, vk::Format format,
                    vk::ImageUsageFlags usage) const;

    [[nodiscard]] BufferBase
    allocate_buffer(VkDeviceSize size, bool host_visible,
                    vk::BufferUsageFlags usage,
                    vk::SharingMode sharing_mode) const;

  private:
    Allocator(std::shared_ptr<const Device> device, VmaAllocator allocator);

    struct Impl {
        std::shared_ptr<const Device> device;
        VmaAllocator allocator;

        Impl(std::shared_ptr<const Device> dev, VmaAllocator alloc);
        ~Impl();

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;
        Impl(Impl &&) = delete;
        Impl &operator=(Impl &&) = delete;
    };

    std::shared_ptr<Impl> m_impl;
};

class AllocatorBuilder {
  public:
    AllocatorBuilder(std::shared_ptr<const Instance> instance,
                     std::shared_ptr<const Device> device);

    std::shared_ptr<Allocator> build();

  private:
    std::shared_ptr<const Instance> m_instance;
    std::shared_ptr<const Device> m_device;
};

// ---- AllocateBufferUtils ----

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

// ---- BufferList ----

template <typename T, bool HostVisible, VkBufferUsageFlags flags>
class BufferList {
  public:
    struct BufferInfo {
        std::shared_ptr<Buffer<T, HostVisible, flags>> buffer;
        std::size_t offset;
    };

    BufferList(std::shared_ptr<const Allocator> allocator) noexcept
        : m_allocator{std::move(allocator)} {}

    BufferInfo create_buffer(std::size_t size, std::size_t alignment = 1) {
        constexpr std::size_t buffer_size = 1 << 24;

        auto align_up = [](std::size_t value, std::size_t align) {
            return (value + align - 1) & ~(align - 1);
        };

        auto has_enough_place = [size, alignment,
                                 align_up](auto &buffer_and_offset) {
            std::size_t aligned_offset =
                align_up(buffer_and_offset.offset, alignment);
            return buffer_and_offset.buffer->size() >= size + aligned_offset;
        };
        auto it = std::ranges::find_if(m_buffer_list, has_enough_place);
        if (it != m_buffer_list.end()) {
            std::size_t aligned_offset = align_up(it->offset, alignment);
            BufferInfo result{it->buffer, aligned_offset};
            it->offset = aligned_offset + size;
            return result;
        }
        auto buffer = std::make_shared<Buffer<T, HostVisible, flags>>(
            vw::create_buffer<T, HostVisible, flags>(
                *m_allocator, std::max(buffer_size, size)));
        auto &buffer_and_offset =
            m_buffer_list.emplace_back(BufferAndOffset{buffer, size});
        return {buffer_and_offset.buffer, 0};
    }

  private:
    struct BufferAndOffset {
        std::shared_ptr<Buffer<T, HostVisible, flags>> buffer;
        std::size_t offset;
    };

    std::shared_ptr<const Allocator> m_allocator;
    std::vector<BufferAndOffset> m_buffer_list;
};

using IndexBufferList = BufferList<uint32_t, false, IndexBufferUsage>;

// ---- StagingBufferManager ----

class StagingBufferManager {
  public:
    StagingBufferManager(std::shared_ptr<const Device> device,
                         std::shared_ptr<const Allocator> allocator);

    [[nodiscard]] vk::CommandBuffer fill_command_buffer();

    template <typename T, bool HostVisible, VkBufferUsageFlags Usage>
    void fill_buffer(std::span<const T> data,
                     const Buffer<T, HostVisible, Usage> &buffer,
                     uint32_t offset_dst_buffer) {
        auto [staging_buffer, offset_src] =
            m_staging_buffers.create_buffer(data.size_bytes());

        static_assert((Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) ==
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        staging_buffer->write_bytes(data, offset_src);

        auto function = [src_buffer_handle = staging_buffer->handle(), &buffer,
                         size = data.size_bytes(),
                         offset_dst = offset_dst_buffer * sizeof(T),
                         offset_src = offset_src * sizeof(std::byte)](
                            vk::CommandBuffer command_buffer) {
            const auto region = vk::BufferCopy()
                                    .setSrcOffset(offset_src)
                                    .setDstOffset(offset_dst)
                                    .setSize(size);
            command_buffer.copyBuffer(src_buffer_handle, buffer.handle(),
                                      region);
        };

        m_transfer_functions.emplace_back(function);
    }

    [[nodiscard]] CombinedImage
    stage_image_from_path(const std::filesystem::path &path, bool mipmaps);

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    CommandPool m_command_pool;
    BufferList<std::byte, true, VK_BUFFER_USAGE_TRANSFER_SRC_BIT>
        m_staging_buffers;

    std::vector<std::function<void(vk::CommandBuffer)>> m_transfer_functions;
    std::shared_ptr<const Sampler> m_sampler;
};

// ---- Barrier functions ----

void execute_image_barrier_undefined_to_transfer_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_undefined_to_general(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_transfer_dst_to_src(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    MipLevel mip_level);

void execute_image_barrier_transfer_src_to_dst(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_transfer_dst_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image);

void execute_image_barrier_general_to_sampled(
    vk::CommandBuffer cmd_buffer, const std::shared_ptr<const Image> &image,
    vk::PipelineStageFlags2 srcStage);

void execute_image_transition(vk::CommandBuffer cmd_buffer,
                              const std::shared_ptr<const Image> &image,
                              vk::ImageLayout oldLayout,
                              vk::ImageLayout newLayout);

// ---- Transfer ----

class Transfer {
  public:
    Transfer() = default;

    void
    blit(vk::CommandBuffer cmd, const std::shared_ptr<const Image> &src,
         const std::shared_ptr<const Image> &dst,
         std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt,
         std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt,
         vk::Filter filter = vk::Filter::eLinear);

    void copyBuffer(vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst,
                    vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
                    vk::DeviceSize size);

    void copyBufferToImage(
        vk::CommandBuffer cmd, vk::Buffer src,
        const std::shared_ptr<const Image> &dst, vk::DeviceSize srcOffset,
        std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt);

    void copyImageToBuffer(
        vk::CommandBuffer cmd, const std::shared_ptr<const Image> &src,
        vk::Buffer dst, vk::DeviceSize dstOffset,
        std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt);

    [[nodiscard]] Barrier::ResourceTracker &resourceTracker() {
        return m_resourceTracker;
    }

    void
    saveToFile(vk::CommandBuffer cmd, const Allocator &allocator, Queue &queue,
               const std::shared_ptr<const Image> &image,
               const std::filesystem::path &path,
               vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR);

  private:
    Barrier::ResourceTracker m_resourceTracker;

    vk::ImageSubresourceRange
    getFullSubresourceRange(const std::shared_ptr<const Image> &image) const;
};

// ---- UniformBufferAllocator ----

template <typename T> struct UniformBufferChunk {
    vk::Buffer handle{};
    vk::DeviceSize offset{};
    vk::DeviceSize size{};
    uint32_t index{};

    std::shared_ptr<Buffer<std::byte, true, UniformBufferUsage>>
        buffer_ref{};

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
    vk::DeviceSize chunkOffset = m_allocations[index].offset;

    if (m_allocations[index].size > alignedSize) {
        vk::DeviceSize remainingSize = m_allocations[index].size - alignedSize;
        vk::DeviceSize newOffset = chunkOffset + alignedSize;

        m_allocations[index].size = alignedSize;
        m_allocations[index].free = false;

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
