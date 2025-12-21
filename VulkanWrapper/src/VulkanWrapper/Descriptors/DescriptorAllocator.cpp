#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"

#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"

constexpr int DESCRIPTOR_ALLOCATOR_RESERVE_SIZE = 20;

namespace vw {
DescriptorAllocator::DescriptorAllocator() {
    m_bufferUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
    m_imageUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
    m_samplerUpdate.reserve(DESCRIPTOR_ALLOCATOR_RESERVE_SIZE);
}

void DescriptorAllocator::add_uniform_buffer(int binding, vk::Buffer buffer,
                                             vk::DeviceSize offset,
                                             vk::DeviceSize size,
                                             vk::PipelineStageFlags2 stage,
                                             vk::AccessFlags2 access,
                                             uint32_t array_element) {
    auto &[info, write, stageFlags, accessFlags] =
        m_bufferUpdate.emplace_back();
    info =
        vk::DescriptorBufferInfo().setBuffer(buffer).setOffset(offset).setRange(
            size);
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_storage_buffer(int binding, vk::Buffer buffer,
                                             vk::DeviceSize offset,
                                             vk::DeviceSize size,
                                             vk::PipelineStageFlags2 stage,
                                             vk::AccessFlags2 access,
                                             uint32_t array_element) {
    auto &[info, write, stageFlags, accessFlags] =
        m_bufferUpdate.emplace_back();
    info =
        vk::DescriptorBufferInfo().setBuffer(buffer).setOffset(offset).setRange(
            size);
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_combined_image(
    int binding, const CombinedImage &combined_image,
    vk::PipelineStageFlags2 stage, vk::AccessFlags2 access,
    uint32_t array_element) {
    auto &[info, write, image, range, stageFlags, accessFlags] =
        m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
               .setImageView(combined_image.image_view())
               .setSampler(combined_image.sampler());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
    image = combined_image.image();
    range = combined_image.subresource_range();
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_storage_image(int binding,
                                            const ImageView &image_view,
                                            vk::PipelineStageFlags2 stage,
                                            vk::AccessFlags2 access,
                                            uint32_t array_element) {
    auto &[info, write, image, range, stageFlags, accessFlags] =
        m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eGeneral)
               .setImageView(image_view.handle());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
    image = image_view.image()->handle();
    range = image_view.subresource_range();
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_acceleration_structure(
    int binding, vk::AccelerationStructureKHR tlas,
    vk::PipelineStageFlags2 stage, vk::AccessFlags2 access,
    uint32_t array_element) {
    auto &[accelerationStructure, info, write, stageFlags, accessFlags] =
        m_accelerationStructureUpdate.emplace();

    accelerationStructure = tlas;

    info = vk::WriteDescriptorSetAccelerationStructureKHR()
               .setAccelerationStructures(accelerationStructure);
    write =
        vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
            .setDstBinding(binding)
            .setDstArrayElement(array_element)
            .setPNext(&write); // Use pNext to pass the acceleration structure
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_input_attachment(
    int binding, std::shared_ptr<const ImageView> image_view,
    vk::PipelineStageFlags2 stage, vk::AccessFlags2 access,
    uint32_t array_element) {
    auto &[info, write, image, range, stageFlags, accessFlags] =
        m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
               .setImageView(image_view->handle());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eInputAttachment)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
    image = image_view->image()->handle();
    range = image_view->subresource_range();
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_sampler(int binding, vk::Sampler sampler,
                                      uint32_t array_element) {
    auto &[info, write] = m_samplerUpdate.emplace_back();
    info = vk::DescriptorImageInfo().setSampler(sampler);
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eSampler)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
}

void DescriptorAllocator::add_sampled_image(int binding,
                                            const ImageView &image_view,
                                            vk::PipelineStageFlags2 stage,
                                            vk::AccessFlags2 access,
                                            uint32_t array_element) {
    auto &[info, write, image, range, stageFlags, accessFlags] =
        m_imageUpdate.emplace_back();
    info = vk::DescriptorImageInfo()
               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
               .setImageView(image_view.handle());
    write = vk::WriteDescriptorSet()
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eSampledImage)
                .setDstBinding(binding)
                .setDstArrayElement(array_element);
    image = image_view.image()->handle();
    range = image_view.subresource_range();
    stageFlags = stage;
    accessFlags = access;
}

void DescriptorAllocator::add_sampled_image(
    int binding, std::shared_ptr<const ImageView> image_view,
    vk::PipelineStageFlags2 stage, vk::AccessFlags2 access,
    uint32_t array_element) {
    add_sampled_image(binding, *image_view, stage, access, array_element);
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

    for (const auto &samplerUpdate : m_samplerUpdate) {
        auto writer = samplerUpdate.write;
        writer.setImageInfo(samplerUpdate.info);
        writers.push_back(writer);
    }

    if (m_accelerationStructureUpdate) {
        auto writer = m_accelerationStructureUpdate->write;
        writer.setPNext(&m_accelerationStructureUpdate->info);
        writers.push_back(writer);
    }

    return writers;
}

std::vector<Barrier::ResourceState> DescriptorAllocator::get_resources() const {
    std::vector<Barrier::ResourceState> resources;

    // Add buffer resources
    for (const auto &bufferUpdate : m_bufferUpdate) {
        Barrier::BufferState bufferState;
        bufferState.buffer = bufferUpdate.info.buffer;
        bufferState.offset = bufferUpdate.info.offset;
        bufferState.size = bufferUpdate.info.range;
        bufferState.stage = bufferUpdate.stage;
        bufferState.access = bufferUpdate.access;
        resources.push_back(bufferState);
    }

    // Add image resources
    for (const auto &imageUpdate : m_imageUpdate) {
        Barrier::ImageState imageState;
        imageState.image = imageUpdate.image;
        imageState.subresourceRange = imageUpdate.subresource_range;
        imageState.layout = imageUpdate.info.imageLayout;
        imageState.stage = imageUpdate.stage;
        imageState.access = imageUpdate.access;
        resources.push_back(imageState);
    }

    // Add acceleration structure resources
    if (m_accelerationStructureUpdate) {
        Barrier::AccelerationStructureState asState;
        asState.handle = m_accelerationStructureUpdate->accelerationStructure;
        asState.stage = m_accelerationStructureUpdate->stage;
        asState.access = m_accelerationStructureUpdate->access;
        resources.push_back(asState);
    }

    return resources;
}
} // namespace vw
