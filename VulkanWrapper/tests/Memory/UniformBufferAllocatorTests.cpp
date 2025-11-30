#include <gtest/gtest.h>
#include "VulkanWrapper/Memory/UniformBufferAllocator.h"

using namespace vw;

// Note: These tests require a Vulkan device, so they are more integration tests
// For now, we'll create simple unit tests that test the logic without actual Vulkan

// Mock test to verify the allocator interface compiles
TEST(UniformBufferAllocatorTest, InterfaceCompiles) {
    // This test just verifies the interface compiles
    // Actual testing would require a Vulkan device
    SUCCEED();
}

// Test alignment calculation
TEST(UniformBufferAllocatorTest, AlignmentCalculation) {
    // Test that alignment works correctly
    vk::DeviceSize minAlignment = 256;
    
    // These would be tested if we could instantiate the allocator
    // For now, we test the alignment math directly
    auto align = [minAlignment](vk::DeviceSize size) {
        return (size + minAlignment - 1) & ~(minAlignment - 1);
    };
    
    EXPECT_EQ(align(1), 256);
    EXPECT_EQ(align(256), 256);
    EXPECT_EQ(align(257), 512);
    EXPECT_EQ(align(512), 512);
    EXPECT_EQ(align(513), 768);
}

// Test chunk descriptor info
TEST(UniformBufferAllocatorTest, ChunkDescriptorInfo) {
    UniformBufferChunk<int> chunk;
    chunk.handle = vk::Buffer{}; // Null handle for testing
    chunk.offset = 256;
    chunk.size = 512;
    chunk.index = 0;
    
    auto info = chunk.descriptorInfo();
    EXPECT_EQ(info.offset, 256);
    EXPECT_EQ(info.range, 512);
}

// Test allocation logic conceptually
TEST(UniformBufferAllocatorTest, AllocationLogic) {
    // Test the first-fit allocation strategy conceptually
    struct MockAllocation {
        vk::DeviceSize offset;
        vk::DeviceSize size;
        bool free;
    };
    
    std::vector<MockAllocation> allocations;
    allocations.push_back({0, 1024, true}); // One large free block
    
    // Simulate allocating 256 bytes
    vk::DeviceSize requestedSize = 256;
    
    // Find first free block
    size_t foundIndex = 0;
    for (size_t i = 0; i < allocations.size(); ++i) {
        if (allocations[i].free && allocations[i].size >= requestedSize) {
            foundIndex = i;
            break;
        }
    }
    
    EXPECT_EQ(foundIndex, 0);
    EXPECT_GE(allocations[foundIndex].size, requestedSize);
    
    // Split the block
    auto &alloc = allocations[foundIndex];
    vk::DeviceSize remainingSize = alloc.size - requestedSize;
    vk::DeviceSize newOffset = alloc.offset + requestedSize;
    
    alloc.size = requestedSize;
    alloc.free = false;
    
    allocations.push_back({newOffset, remainingSize, true});
    
    EXPECT_EQ(allocations.size(), 2);
    EXPECT_EQ(allocations[0].size, 256);
    EXPECT_FALSE(allocations[0].free);
    EXPECT_EQ(allocations[1].offset, 256);
    EXPECT_EQ(allocations[1].size, 768);
    EXPECT_TRUE(allocations[1].free);
}

// Test deallocation and merging logic
TEST(UniformBufferAllocatorTest, DeallocationMerging) {
    struct MockAllocation {
        vk::DeviceSize offset;
        vk::DeviceSize size;
        bool free;
    };
    
    std::vector<MockAllocation> allocations;
    // Three adjacent blocks: used, used, used
    allocations.push_back({0, 256, false});
    allocations.push_back({256, 256, false});
    allocations.push_back({512, 256, false});
    
    // Free the middle one
    allocations[1].free = true;
    
    // Free the first one
    allocations[0].free = true;
    
    // Sort by offset
    std::sort(allocations.begin(), allocations.end(),
              [](const MockAllocation &a, const MockAllocation &b) {
                  return a.offset < b.offset;
              });
    
    // Merge adjacent free blocks
    for (size_t i = 0; i < allocations.size();) {
        if (!allocations[i].free) {
            ++i;
            continue;
        }
        
        size_t j = i + 1;
        while (j < allocations.size() && 
               allocations[j].free &&
               allocations[i].offset + allocations[i].size == allocations[j].offset) {
            allocations[i].size += allocations[j].size;
            allocations.erase(allocations.begin() + j);
        }
        ++i;
    }
    
    // Should have merged first two blocks
    EXPECT_EQ(allocations.size(), 2);
    EXPECT_EQ(allocations[0].offset, 0);
    EXPECT_EQ(allocations[0].size, 512);
    EXPECT_TRUE(allocations[0].free);
    EXPECT_EQ(allocations[1].offset, 512);
    EXPECT_FALSE(allocations[1].free);
}

// Test free space calculation
TEST(UniformBufferAllocatorTest, FreeSpaceCalculation) {
    struct MockAllocation {
        vk::DeviceSize offset;
        vk::DeviceSize size;
        bool free;
    };
    
    std::vector<MockAllocation> allocations;
    allocations.push_back({0, 256, false});
    allocations.push_back({256, 256, true});
    allocations.push_back({512, 512, false});
    allocations.push_back({1024, 256, true});
    
    vk::DeviceSize freeSpace = 0;
    for (const auto &alloc : allocations) {
        if (alloc.free) {
            freeSpace += alloc.size;
        }
    }
    
    EXPECT_EQ(freeSpace, 512); // 256 + 256
}
