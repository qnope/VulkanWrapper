#pragma once

namespace vw {
class DescriptorAllocator {
  public:
    DescriptorAllocator();

    void add_buffer(int binding, vk::DescriptorType type, vk::Buffer buffer,
                    vk::DeviceSize offset, vk::DeviceSize size);

    [[nodiscard]] std::vector<vk::WriteDescriptorSet>
    get_write_descriptors() const;

    bool operator==(const DescriptorAllocator &) const = default;

  private:
    struct BufferUpdate {
        vk::DescriptorBufferInfo info;
        vk::WriteDescriptorSet write;
        bool operator==(const BufferUpdate &) const = default;
    };

    struct ImageUpdate {
        vk::DescriptorImageInfo info;
        vk::WriteDescriptorSet write;
        bool operator==(const ImageUpdate &) const = default;
    };

    std::vector<BufferUpdate> m_bufferUpdate;
    std::vector<ImageUpdate> m_imageUpdate;
};

} // namespace vw

template <> struct std::hash<vw::DescriptorAllocator> {
    std::size_t
    operator()(const vw::DescriptorAllocator &allocator) const noexcept {
        return 0;
    }
};
