#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"

namespace vw {
class DescriptorAllocator {
  public:
    DescriptorAllocator();

    void add_uniform_buffer(int binding, vk::Buffer buffer,
                            vk::DeviceSize offset, vk::DeviceSize size,
                            vk::PipelineStageFlags2 stage,
                            vk::AccessFlags2 access);

    void add_combined_image(int binding, const CombinedImage &image,
                            vk::PipelineStageFlags2 stage,
                            vk::AccessFlags2 access);

    void add_storage_image(int binding, const ImageView &image_view,
                           vk::PipelineStageFlags2 stage,
                           vk::AccessFlags2 access);

    void add_input_attachment(int binding,
                              std::shared_ptr<const ImageView> image_view,
                              vk::PipelineStageFlags2 stage,
                              vk::AccessFlags2 access);

    void add_acceleration_structure(int binding,
                                    vk::AccelerationStructureKHR tlas,
                                    vk::PipelineStageFlags2 stage,
                                    vk::AccessFlags2 access);

    [[nodiscard]] std::vector<vk::WriteDescriptorSet>
    get_write_descriptors() const;

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

    bool operator==(const DescriptorAllocator &) const = default;

  private:
    struct BufferUpdate {
        vk::DescriptorBufferInfo info;
        vk::WriteDescriptorSet write;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        bool operator==(const BufferUpdate &) const = default;
    };

    struct ImageUpdate {
        vk::DescriptorImageInfo info;
        vk::WriteDescriptorSet write;
        vk::Image image;
        vk::ImageSubresourceRange subresource_range;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        bool operator==(const ImageUpdate &) const = default;
    };

    struct AccelerationStructureUpdate {
        vk::AccelerationStructureKHR accelerationStructure;
        vk::WriteDescriptorSetAccelerationStructureKHR info;
        vk::WriteDescriptorSet write;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        bool operator==(const AccelerationStructureUpdate &) const = default;
    };

    std::vector<BufferUpdate> m_bufferUpdate;
    std::vector<ImageUpdate> m_imageUpdate;
    std::optional<AccelerationStructureUpdate> m_accelerationStructureUpdate;
};

} // namespace vw

template <> struct std::hash<vw::DescriptorAllocator> {
    std::size_t
    operator()(const vw::DescriptorAllocator &allocator) const noexcept {
        return 0;
    }
};
