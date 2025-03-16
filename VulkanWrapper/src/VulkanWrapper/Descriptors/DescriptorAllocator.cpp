#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"

constexpr int DESCRIPTOR_ALLOCATOR_RESERVE_SIZE = 20;

namespace vw {
DescriptorAllocator::DescriptorAllocator() {
    m_bufferUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
    m_imageUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
}

void DescriptorAllocator::add_buffer(int binding, vk::DescriptorType type,
                                     vk::Buffer buffer, vk::DeviceSize offset,
                                     vk::DeviceSize size) {
    auto &[info, write] = m_bufferUpdate.emplace_back();
    info =
        vk::DescriptorBufferInfo().setBuffer(buffer).setOffset(offset).setRange(
            size);
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(type)
                .setDstBinding(binding)
                .setDstArrayElement(0);
}

std::vector<vk::WriteDescriptorSet>
DescriptorAllocator::get_write_descriptors() const {
    std::vector<vk::WriteDescriptorSet> writers;

    for (const auto &bufferUpdate : m_bufferUpdate) {
        auto writer = bufferUpdate.write;
        writer.setBufferInfo(bufferUpdate.info);
        writers.push_back(writer);
    }

    for (const auto &imageUpdate : m_imageUpdate) {
        auto writer = imageUpdate.write;
        writer.setImageInfo(imageUpdate.info);
        writers.push_back(writer);
    }

    return writers;
}
} // namespace vw
