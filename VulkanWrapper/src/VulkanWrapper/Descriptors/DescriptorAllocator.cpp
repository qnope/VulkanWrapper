#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"

namespace vw {
DescriptorAllocator::DescriptorAllocator() {
    m_bufferUpdate.reserve(20);
    m_imageUpdate.reserve(20);
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

    for (auto &bufferUpdate : m_bufferUpdate) {
        auto writer = bufferUpdate.write;
        writer.setBufferInfo(bufferUpdate.info);
        writers.push_back(writer);
    }

    for (auto &imageUpdate : m_imageUpdate) {
        auto writer = imageUpdate.write;
        writer.setImageInfo(imageUpdate.info);
        writers.push_back(writer);
    }

    return writers;
}
} // namespace vw
