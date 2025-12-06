#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/BufferUsage.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vk_mem_alloc.h>
#include <cstring>

namespace vw {

class BufferBase : public ObjectWithHandle<vk::Buffer> {
  public:
    BufferBase(std::shared_ptr<const Device> device,
               std::shared_ptr<const Allocator> allocator,
               vk::Buffer buffer, VmaAllocation allocation, VkDeviceSize size);

    BufferBase(const BufferBase &) = delete;
    BufferBase &operator=(const BufferBase &) = delete;

    BufferBase(BufferBase &&other) noexcept = default;
    BufferBase &operator=(BufferBase &&other) noexcept = default;

    [[nodiscard]] VkDeviceSize size_bytes() const noexcept;
    [[nodiscard]] vk::DeviceAddress device_address() const noexcept;

    void generic_copy(const void *data, VkDeviceSize size, VkDeviceSize offset);
    [[nodiscard]] std::vector<std::byte> generic_as_vector(VkDeviceSize offset, VkDeviceSize size) const;

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

    void copy(std::span<const T> span, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::generic_copy(span.data(), span.size_bytes(),
                                 offset * sizeof(T));
    }

    void copy(const T &object, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::generic_copy(&object, sizeof(T), offset * sizeof(T));
    }

    template <typename U>
    void generic_copy(std::span<const U> span, std::size_t offset) {
        BufferBase::generic_copy(span.data(), span.size_bytes(),
                                 offset * sizeof(T));
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_bytes() / sizeof(T);
    }

    [[nodiscard]] std::vector<T> as_vector(std::size_t offset, std::size_t count) const
        requires(HostVisible)
    {
        auto bytes = BufferBase::generic_as_vector(offset * sizeof(T), count * sizeof(T));
        std::vector<T> result(count);
        std::memcpy(result.data(), bytes.data(), bytes.size());
        return result;
    }
};

} // namespace vw
