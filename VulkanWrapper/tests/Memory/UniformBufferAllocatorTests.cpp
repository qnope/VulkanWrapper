#include <gtest/gtest.h>
#include "VulkanWrapper/Memory/UniformBufferAllocator.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "utils/create_gpu.hpp"
#include <glm/glm.hpp>
#include <optional>
#include <cstring>

TEST(UniformBufferAllocatorTest, CreateAllocator) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    SUCCEED();
}

TEST(UniformBufferAllocatorTest, AllocateChunk) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    
    auto chunk = uboAllocator.allocate<float>();
    ASSERT_TRUE(chunk.has_value());
    SUCCEED();
}

TEST(UniformBufferAllocatorTest, CopyToChunk) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    
    auto chunk = uboAllocator.allocate<float>();
    ASSERT_TRUE(chunk.has_value());
    
    float value = 123.456f;
    chunk->copy(value);
    SUCCEED();
}

TEST(UniformBufferAllocatorTest, AllocateAndCopyFloat) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    
    auto chunk = uboAllocator.allocate<float>();
    ASSERT_TRUE(chunk.has_value());
    
    float value = 123.456f;
    chunk->copy(value);
    
    const auto& buffer = uboAllocator.buffer_ref();
    // Read only sizeof(float) bytes, not the aligned size
    auto data = buffer.as_vector(static_cast<std::size_t>(chunk->offset), static_cast<std::size_t>(sizeof(float)));
    
    ASSERT_EQ(data.size(), sizeof(float));
    float loadedValue;
    std::memcpy(&loadedValue, data.data(), sizeof(float));
    EXPECT_FLOAT_EQ(loadedValue, value);
}

TEST(UniformBufferAllocatorTest, AllocateSameStructureMultipleTimes) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    
    struct TestStruct {
        float x;
        float y;
        float z;
        int id;
    };
    
    // Allocate multiple chunks of the same structure
    auto chunk1 = uboAllocator.allocate<TestStruct>();
    ASSERT_TRUE(chunk1.has_value());
    
    auto chunk2 = uboAllocator.allocate<TestStruct>();
    ASSERT_TRUE(chunk2.has_value());
    
    auto chunk3 = uboAllocator.allocate<TestStruct>();
    ASSERT_TRUE(chunk3.has_value());
    
    // Set different values
    TestStruct value1{1.0f, 2.0f, 3.0f, 100};
    TestStruct value2{4.0f, 5.0f, 6.0f, 200};
    TestStruct value3{7.0f, 8.0f, 9.0f, 300};
    
    chunk1->copy(value1);
    chunk2->copy(value2);
    chunk3->copy(value3);
    
    // Retrieve and verify each one
    const auto& buffer = uboAllocator.buffer_ref();
    
    auto data1 = buffer.as_vector(static_cast<std::size_t>(chunk1->offset), sizeof(TestStruct));
    auto data2 = buffer.as_vector(static_cast<std::size_t>(chunk2->offset), sizeof(TestStruct));
    auto data3 = buffer.as_vector(static_cast<std::size_t>(chunk3->offset), sizeof(TestStruct));
    
    ASSERT_EQ(data1.size(), sizeof(TestStruct));
    ASSERT_EQ(data2.size(), sizeof(TestStruct));
    ASSERT_EQ(data3.size(), sizeof(TestStruct));
    
    TestStruct loaded1, loaded2, loaded3;
    std::memcpy(&loaded1, data1.data(), sizeof(TestStruct));
    std::memcpy(&loaded2, data2.data(), sizeof(TestStruct));
    std::memcpy(&loaded3, data3.data(), sizeof(TestStruct));
    
    EXPECT_FLOAT_EQ(loaded1.x, value1.x);
    EXPECT_FLOAT_EQ(loaded1.y, value1.y);
    EXPECT_FLOAT_EQ(loaded1.z, value1.z);
    EXPECT_EQ(loaded1.id, value1.id);
    
    EXPECT_FLOAT_EQ(loaded2.x, value2.x);
    EXPECT_FLOAT_EQ(loaded2.y, value2.y);
    EXPECT_FLOAT_EQ(loaded2.z, value2.z);
    EXPECT_EQ(loaded2.id, value2.id);
    
    EXPECT_FLOAT_EQ(loaded3.x, value3.x);
    EXPECT_FLOAT_EQ(loaded3.y, value3.y);
    EXPECT_FLOAT_EQ(loaded3.z, value3.z);
    EXPECT_EQ(loaded3.id, value3.id);
}

TEST(UniformBufferAllocatorTest, AllocateDifferentStructures) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    
    struct SmallStruct {
        int32_t value;
    };
    
    struct MediumStruct {
        float x, y, z;
        uint32_t flags;
    };
    
    struct LargeStruct {
        glm::vec4 position;
        glm::vec4 color;
        float intensity;
        int32_t id;
    };
    
    // Allocate different structure types
    auto chunk1 = uboAllocator.allocate<SmallStruct>();
    ASSERT_TRUE(chunk1.has_value());
    
    auto chunk2 = uboAllocator.allocate<MediumStruct>();
    ASSERT_TRUE(chunk2.has_value());
    
    auto chunk3 = uboAllocator.allocate<LargeStruct>();
    ASSERT_TRUE(chunk3.has_value());
    
    // Set values
    SmallStruct value1{42};
    MediumStruct value2{10.5f, 20.5f, 30.5f, 0x12345678};
    LargeStruct value3{
        glm::vec4(1.0f, 2.0f, 3.0f, 4.0f),
        glm::vec4(0.1f, 0.2f, 0.3f, 0.4f),
        99.9f,
        12345
    };
    
    chunk1->copy(value1);
    chunk2->copy(value2);
    chunk3->copy(value3);
    
    // Retrieve and verify each one
    const auto& buffer = uboAllocator.buffer_ref();
    
    auto data1 = buffer.as_vector(static_cast<std::size_t>(chunk1->offset), sizeof(SmallStruct));
    auto data2 = buffer.as_vector(static_cast<std::size_t>(chunk2->offset), sizeof(MediumStruct));
    auto data3 = buffer.as_vector(static_cast<std::size_t>(chunk3->offset), sizeof(LargeStruct));
    
    ASSERT_EQ(data1.size(), sizeof(SmallStruct));
    ASSERT_EQ(data2.size(), sizeof(MediumStruct));
    ASSERT_EQ(data3.size(), sizeof(LargeStruct));
    
    SmallStruct loaded1;
    MediumStruct loaded2;
    LargeStruct loaded3;
    
    std::memcpy(&loaded1, data1.data(), sizeof(SmallStruct));
    std::memcpy(&loaded2, data2.data(), sizeof(MediumStruct));
    std::memcpy(&loaded3, data3.data(), sizeof(LargeStruct));
    
    EXPECT_EQ(loaded1.value, value1.value);
    
    EXPECT_FLOAT_EQ(loaded2.x, value2.x);
    EXPECT_FLOAT_EQ(loaded2.y, value2.y);
    EXPECT_FLOAT_EQ(loaded2.z, value2.z);
    EXPECT_EQ(loaded2.flags, value2.flags);
    
    EXPECT_FLOAT_EQ(loaded3.position.x, value3.position.x);
    EXPECT_FLOAT_EQ(loaded3.position.y, value3.position.y);
    EXPECT_FLOAT_EQ(loaded3.position.z, value3.position.z);
    EXPECT_FLOAT_EQ(loaded3.position.w, value3.position.w);
    EXPECT_FLOAT_EQ(loaded3.color.x, value3.color.x);
    EXPECT_FLOAT_EQ(loaded3.color.y, value3.color.y);
    EXPECT_FLOAT_EQ(loaded3.color.z, value3.color.z);
    EXPECT_FLOAT_EQ(loaded3.color.w, value3.color.w);
    EXPECT_FLOAT_EQ(loaded3.intensity, value3.intensity);
    EXPECT_EQ(loaded3.id, value3.id);
}

TEST(UniformBufferAllocatorTest, AllocateSameStructureWithVector) {
    auto gpu = vw::tests::create_gpu();
    auto allocator = vw::AllocatorBuilder(gpu.instance, gpu.device).build();
    vw::UniformBufferAllocator uboAllocator(allocator, 1024 * 1024);
    
    struct Vec3 {
        float x, y, z;
    };
    
    // Allocate space for multiple Vec3 elements
    constexpr size_t count = 5;
    auto chunk = uboAllocator.allocate<Vec3>(count);
    ASSERT_TRUE(chunk.has_value());
    
    // Create a vector of values
    std::vector<Vec3> values;
    for (size_t i = 0; i < count; ++i) {
        values.push_back({static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)});
    }
    
    // Copy the entire vector
    chunk->copy(std::span<const Vec3>(values));
    
    // Retrieve using as_vector
    const auto& buffer = uboAllocator.buffer_ref();
    auto data = buffer.as_vector(static_cast<std::size_t>(chunk->offset), count * sizeof(Vec3));
    
    ASSERT_EQ(data.size(), count * sizeof(Vec3));
    
    // Verify each element
    for (size_t i = 0; i < count; ++i) {
        Vec3 loaded;
        std::memcpy(&loaded, data.data() + i * sizeof(Vec3), sizeof(Vec3));
        
        EXPECT_FLOAT_EQ(loaded.x, values[i].x);
        EXPECT_FLOAT_EQ(loaded.y, values[i].y);
        EXPECT_FLOAT_EQ(loaded.z, values[i].z);
    }
}
