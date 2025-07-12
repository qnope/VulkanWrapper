#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"

#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"

constexpr int DESCRIPTOR_ALLOCATOR_RESERVE_SIZE = 20;

namespace vw {
DescriptorAllocator::DescriptorAllocator() {
    m_bufferUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
    m_imageUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
}

void DescriptorAllocator::add_uniform_buffer(int binding, vk::Buffer buffer,
                                             vk::DeviceSize offset,
                                             vk::DeviceSize size) {
    auto &[info, write] = m_bufferUpdate.emplace_back();
    info =
        vk::DescriptorBufferInfo().setBuffer(buffer).setOffset(offset).setRange(
            size);
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstBinding(binding)
                .setDstArrayElement(0);
}

void DescriptorAllocator::add_combined_image(int binding,
                                             const CombinedImage &image) {
    auto &[info, write] = m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
               .setImageView(image.image_view())
               .setSampler(image.sampler());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstBinding(binding)
                .setDstArrayElement(0);
}

void DescriptorAllocator::add_combined_image(int binding,
                                             const std::shared_ptr<const ImageView> &image_view) {
    auto &[info, write] = m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
               .setImageView(image_view->handle());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstBinding(binding)
                .setDstArrayElement(0);
}

void DescriptorAllocator::add_combined_image_sampler(int binding,
                                                     const CombinedImage &image) {
    add_combined_image(binding, image);
}

void DescriptorAllocator::add_acceleration_structure(int binding, vk::AccelerationStructureKHR tlas) {
    auto &[info, write] = m_bufferUpdate.emplace_back();
    // For acceleration structures, we use the buffer info but set the descriptor type to acceleration structure
    info = vk::DescriptorBufferInfo(); // Empty buffer info for acceleration structure
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                .setDstBinding(binding)
                .setDstArrayElement(0)
                .setPNext(&tlas); // Use pNext to pass the acceleration structure
}

void DescriptorAllocator::add_input_attachment(
    int binding, std::shared_ptr<const ImageView> image_view) {
    auto &[info, write] = m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
               .setImageView(image_view->handle());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eInputAttachment)
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
