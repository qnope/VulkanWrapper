#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Image/Image.h"
#include "VulkanWrapper/Image/ImageView.h"
#include "VulkanWrapper/Image/Sampler.h"
#include "VulkanWrapper/Image/CombinedImage.h"
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include <gtest/gtest.h>

// DescriptorSetLayoutBuilder Tests

TEST(DescriptorSetLayoutBuilderTest, BuildEmptyLayout) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device).build();

    EXPECT_NE(layout->handle(), nullptr);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithUniformBuffer) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eUniformBuffer);
    EXPECT_EQ(poolSizes[0].descriptorCount, 1);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithMultipleUniformBuffers) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 3)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].descriptorCount, 3);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithSampledImage) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_sampled_image(vk::ShaderStageFlagBits::eFragment, 1)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eSampledImage);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithCombinedImageSampler) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eCombinedImageSampler);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithStorageImage) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_storage_image(vk::ShaderStageFlagBits::eCompute, 1)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eStorageImage);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithInputAttachment) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_input_attachment(vk::ShaderStageFlagBits::eFragment)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eInputAttachment);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithAccelerationStructure) {
    auto &gpu = vw::tests::create_gpu();
    auto layout =
        vw::DescriptorSetLayoutBuilder(gpu.device)
            .with_acceleration_structure(vk::ShaderStageFlagBits::eFragment)
            .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eAccelerationStructureKHR);
}

TEST(DescriptorSetLayoutBuilderTest, BuildWithMultipleBindingTypes) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .with_combined_image(vk::ShaderStageFlagBits::eFragment, 2)
                      .with_storage_image(vk::ShaderStageFlagBits::eCompute, 1)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
    auto poolSizes = layout->get_pool_sizes();
    EXPECT_EQ(poolSizes.size(), 3);
}

TEST(DescriptorSetLayoutBuilderTest, FluentApiChaining) {
    auto &gpu = vw::tests::create_gpu();

    // Test that fluent API chaining works correctly
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment, 1)
                      .with_combined_image(vk::ShaderStageFlagBits::eFragment, 4)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
}

TEST(DescriptorSetLayoutBuilderTest, AllShaderStages) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eAll, 1)
                      .build();

    EXPECT_NE(layout->handle(), nullptr);
}

// DescriptorSetLayout Tests

TEST(DescriptorSetLayoutTest, GetPoolSizesEmpty) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device).build();

    auto poolSizes = layout->get_pool_sizes();
    EXPECT_TRUE(poolSizes.empty());
}

TEST(DescriptorSetLayoutTest, GetPoolSizesAggregatesSameType) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 2)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eFragment, 3)
                      .build();

    auto poolSizes = layout->get_pool_sizes();
    ASSERT_EQ(poolSizes.size(), 1);
    EXPECT_EQ(poolSizes[0].type, vk::DescriptorType::eUniformBuffer);
    EXPECT_EQ(poolSizes[0].descriptorCount, 5);
}

// DescriptorAllocator Tests

TEST(DescriptorAllocatorTest, DefaultConstruction) {
    vw::DescriptorAllocator allocator;
    auto writeDescriptors = allocator.get_write_descriptors();
    EXPECT_TRUE(writeDescriptors.empty());
}

TEST(DescriptorAllocatorTest, AddUniformBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    vw::DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer.handle(), 0, buffer.size_bytes(),
                                 vk::PipelineStageFlagBits2::eVertexShader,
                                 vk::AccessFlagBits2::eUniformRead);

    auto writeDescriptors = allocator.get_write_descriptors();
    ASSERT_EQ(writeDescriptors.size(), 1);
    EXPECT_EQ(writeDescriptors[0].dstBinding, 0);
    EXPECT_EQ(writeDescriptors[0].descriptorType,
              vk::DescriptorType::eUniformBuffer);
}

TEST(DescriptorAllocatorTest, AddMultipleUniformBuffers) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer1 = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);
    auto buffer2 = vw::create_buffer<UniformBuffer>(*gpu.allocator, 50);

    vw::DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer1.handle(), 0, buffer1.size_bytes(),
                                 vk::PipelineStageFlagBits2::eVertexShader,
                                 vk::AccessFlagBits2::eUniformRead);
    allocator.add_uniform_buffer(1, buffer2.handle(), 0, buffer2.size_bytes(),
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eUniformRead);

    auto writeDescriptors = allocator.get_write_descriptors();
    ASSERT_EQ(writeDescriptors.size(), 2);
}

TEST(DescriptorAllocatorTest, AddUniformBufferWithOffset) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    vw::DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer.handle(), 64, 128,
                                 vk::PipelineStageFlagBits2::eVertexShader,
                                 vk::AccessFlagBits2::eUniformRead);

    auto writeDescriptors = allocator.get_write_descriptors();
    ASSERT_EQ(writeDescriptors.size(), 1);
}

TEST(DescriptorAllocatorTest, EqualityOperatorEmptyAllocators) {
    vw::DescriptorAllocator allocator1;
    vw::DescriptorAllocator allocator2;

    EXPECT_EQ(allocator1, allocator2);
}

TEST(DescriptorAllocatorTest, EqualityOperatorSameContent) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    vw::DescriptorAllocator allocator1;
    allocator1.add_uniform_buffer(0, buffer.handle(), 0, buffer.size_bytes(),
                                  vk::PipelineStageFlagBits2::eVertexShader,
                                  vk::AccessFlagBits2::eUniformRead);

    vw::DescriptorAllocator allocator2;
    allocator2.add_uniform_buffer(0, buffer.handle(), 0, buffer.size_bytes(),
                                  vk::PipelineStageFlagBits2::eVertexShader,
                                  vk::AccessFlagBits2::eUniformRead);

    EXPECT_EQ(allocator1, allocator2);
}

TEST(DescriptorAllocatorTest, EqualityOperatorDifferentContent) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer1 = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);
    auto buffer2 = vw::create_buffer<UniformBuffer>(*gpu.allocator, 50);

    vw::DescriptorAllocator allocator1;
    allocator1.add_uniform_buffer(0, buffer1.handle(), 0, buffer1.size_bytes(),
                                  vk::PipelineStageFlagBits2::eVertexShader,
                                  vk::AccessFlagBits2::eUniformRead);

    vw::DescriptorAllocator allocator2;
    allocator2.add_uniform_buffer(0, buffer2.handle(), 0, buffer2.size_bytes(),
                                  vk::PipelineStageFlagBits2::eVertexShader,
                                  vk::AccessFlagBits2::eUniformRead);

    EXPECT_NE(allocator1, allocator2);
}

TEST(DescriptorAllocatorTest, GetResourcesEmpty) {
    vw::DescriptorAllocator allocator;
    auto resources = allocator.get_resources();
    EXPECT_TRUE(resources.empty());
}

TEST(DescriptorAllocatorTest, GetResourcesWithBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    vw::DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer.handle(), 0, buffer.size_bytes(),
                                 vk::PipelineStageFlagBits2::eVertexShader,
                                 vk::AccessFlagBits2::eUniformRead);

    auto resources = allocator.get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST(DescriptorAllocatorTest, HashableForUnorderedMap) {
    vw::DescriptorAllocator allocator;
    std::hash<vw::DescriptorAllocator> hasher;
    // Should not throw
    auto hash = hasher(allocator);
    // Hash should be callable (value doesn't matter for this test)
    (void)hash;
    SUCCEED();
}

// DescriptorPool Tests

TEST(DescriptorPoolTest, Construction) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    vw::DescriptorPool pool(gpu.device, layout);
    EXPECT_EQ(pool.layout(), layout);
}

TEST(DescriptorPoolTest, AllocateSetWithUniformBuffer) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    vw::DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer.handle(), 0, buffer.size_bytes(),
                                 vk::PipelineStageFlagBits2::eVertexShader,
                                 vk::AccessFlagBits2::eUniformRead);

    vw::DescriptorPool pool(gpu.device, layout);
    auto set = pool.allocate_set(allocator);

    EXPECT_NE(set.handle(), nullptr);
}

TEST(DescriptorPoolTest, AllocateSameSetTwiceReturnsCached) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    vw::DescriptorAllocator allocator;
    allocator.add_uniform_buffer(0, buffer.handle(), 0, buffer.size_bytes(),
                                 vk::PipelineStageFlagBits2::eVertexShader,
                                 vk::AccessFlagBits2::eUniformRead);

    vw::DescriptorPool pool(gpu.device, layout);
    auto set1 = pool.allocate_set(allocator);
    auto set2 = pool.allocate_set(allocator);

    // Same allocator should return same cached set
    EXPECT_EQ(set1.handle(), set2.handle());
}

TEST(DescriptorPoolTest, AllocateDifferentSetsReturnsDifferent) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer1 = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);
    auto buffer2 = vw::create_buffer<UniformBuffer>(*gpu.allocator, 50);

    vw::DescriptorAllocator allocator1;
    allocator1.add_uniform_buffer(0, buffer1.handle(), 0, buffer1.size_bytes(),
                                  vk::PipelineStageFlagBits2::eVertexShader,
                                  vk::AccessFlagBits2::eUniformRead);

    vw::DescriptorAllocator allocator2;
    allocator2.add_uniform_buffer(0, buffer2.handle(), 0, buffer2.size_bytes(),
                                  vk::PipelineStageFlagBits2::eVertexShader,
                                  vk::AccessFlagBits2::eUniformRead);

    vw::DescriptorPool pool(gpu.device, layout);
    auto set1 = pool.allocate_set(allocator1);
    auto set2 = pool.allocate_set(allocator2);

    // Different allocators should return different sets
    EXPECT_NE(set1.handle(), set2.handle());
}

TEST(DescriptorPoolTest, AllocateManySets) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;

    vw::DescriptorPool pool(gpu.device, layout);

    // Allocate more than MAX_DESCRIPTOR_SET_BY_POOL (16) to test pool expansion
    std::vector<vw::DescriptorSet> sets;
    for (int i = 0; i < 20; ++i) {
        auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100 + i);
        vw::DescriptorAllocator allocator;
        allocator.add_uniform_buffer(
            0, buffer.handle(), 0, buffer.size_bytes(),
            vk::PipelineStageFlagBits2::eVertexShader,
            vk::AccessFlagBits2::eUniformRead);
        sets.push_back(pool.allocate_set(allocator));
    }

    // All sets should be valid
    for (const auto &set : sets) {
        EXPECT_NE(set.handle(), nullptr);
    }
}

TEST(DescriptorPoolTest, LayoutAccessor) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    vw::DescriptorPool pool(gpu.device, layout);

    EXPECT_EQ(pool.layout(), layout);
    EXPECT_EQ(pool.layout()->handle(), layout->handle());
}

// DescriptorPoolBuilder Tests

TEST(DescriptorPoolBuilderTest, BuildCreatesPool) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device)
                      .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                      .build();

    auto pool = vw::DescriptorPoolBuilder(gpu.device, layout).build();

    EXPECT_EQ(pool.layout(), layout);
}
