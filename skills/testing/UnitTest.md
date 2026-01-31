# Unit Testing Guide

This guide covers writing unit tests for the VulkanWrapper project using Google Test (GTest).

## Table of Contents

1. [Test Categories](#test-categories)
2. [GPU Singleton Pattern](#gpu-singleton-pattern)
3. [Writing Basic Unit Tests](#writing-basic-unit-tests)
4. [Testing with GPU Resources](#testing-with-gpu-resources)
5. [Buffer Testing Patterns](#buffer-testing-patterns)
6. [Test Organization](#test-organization)
7. [Best Practices](#best-practices)

---

## Test Categories

Unit tests in VulkanWrapper fall into two categories:

### Pure Logic Tests (No GPU)

These tests verify algorithms and data structures without requiring GPU access:

```cpp
#include "VulkanWrapper/Memory/IntervalSet.h"
#include <gtest/gtest.h>

TEST(BufferIntervalSetTest, AddOverlappingIntervals_Merge) {
    BufferIntervalSet set;
    set.add(BufferInterval(0, 100));
    set.add(BufferInterval(50, 100));

    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.intervals()[0], BufferInterval(0, 150));
}
```

### GPU-Dependent Tests

These tests require Vulkan device access and use the `create_gpu()` singleton:

```cpp
#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/Buffer.h"
#include <gtest/gtest.h>

TEST(BufferTest, CreateUniformBuffer) {
    auto &gpu = vw::tests::create_gpu();
    using UniformBuffer = vw::Buffer<float, false, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<UniformBuffer>(*gpu.allocator, 100);

    EXPECT_EQ(buffer.size(), 100);
}
```

---

## GPU Singleton Pattern

The `create_gpu()` function provides a shared GPU instance across all tests:

### Header (`utils/create_gpu.hpp`)

```cpp
#pragma once

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include <memory>

namespace vw::tests {

struct GPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;

    vw::Queue& queue() { return device->graphicsQueue(); }
};

GPU& create_gpu();

} // namespace vw::tests
```

### Usage

```cpp
TEST(MyTest, SomeTest) {
    auto &gpu = vw::tests::create_gpu();

    // Access device
    auto device = gpu.device;

    // Access allocator
    auto allocator = gpu.allocator;

    // Access graphics queue
    auto &queue = gpu.queue();
}
```

### Key Properties

- **Singleton**: Created once, reused across all tests
- **Intentional leak**: Avoids C++ static destruction order issues
- **Debug enabled**: Validation layers are active
- **Vulkan 1.3**: Modern API features available

---

## Writing Basic Unit Tests

### Simple Value Tests

```cpp
TEST(MyClassTest, PropertyReturnsCorrectValue) {
    MyClass obj(42);
    EXPECT_EQ(obj.value(), 42);
}
```

### Floating-Point Comparisons

Use `EXPECT_NEAR` for floating-point values:

```cpp
TEST(MathTest, ComputesSqrt) {
    float result = compute_sqrt(2.0f);
    EXPECT_NEAR(result, 1.414f, 0.001f);
}
```

### Exception Testing

```cpp
TEST(ErrorTest, ThrowsOnInvalidInput) {
    EXPECT_THROW(create_buffer(nullptr, 100), vw::LogicException);
}
```

### Container Tests

```cpp
TEST(ContainerTest, VectorHasExpectedElements) {
    std::vector<int> result = get_values();

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
}
```

---

## Testing with GPU Resources

### Creating Test Images

```cpp
TEST(ImageTest, CreateImage2D) {
    auto &gpu = vw::tests::create_gpu();

    constexpr Width width{64};
    constexpr Height height{64};

    auto image = gpu.allocator->create_image_2D(
        width, height, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferDst);

    EXPECT_EQ(image->extent2D().width, 64);
    EXPECT_EQ(image->extent2D().height, 64);
}
```

### Creating Command Buffers

```cpp
TEST(CommandTest, RecordCommands) {
    auto &gpu = vw::tests::create_gpu();

    auto cmdPool = vw::CommandPoolBuilder(gpu.device).build();
    auto cmd = cmdPool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Record commands...

    std::ignore = cmd.end();

    gpu.queue().enqueue_command_buffer(cmd);
    gpu.queue().submit({}, {}, {}).wait();
}
```

---

## Buffer Testing Patterns

### Read/Write Testing

```cpp
TEST(BufferTest, WriteAndReadBack) {
    auto &gpu = vw::tests::create_gpu();

    // Create host-visible buffer (HostVisible = true)
    using HostBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 10);

    // Write data
    std::vector<float> values = {1.0f, 2.0f, 3.0f};
    buffer.write(std::span<const float>(values), 0);

    // Read back and verify
    auto retrieved = buffer.read_as_vector(0, values.size());

    ASSERT_EQ(retrieved.size(), values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], values[i]);
    }
}
```

### Staging Buffer Pattern

For GPU-only buffers, use staging buffers:

```cpp
TEST(BufferTest, GPUBufferViaStaging) {
    auto &gpu = vw::tests::create_gpu();

    // Create GPU-only buffer
    using GPUBuffer = vw::Buffer<float, false, vw::StorageBufferUsage>;
    auto gpu_buffer = vw::create_buffer<GPUBuffer>(*gpu.allocator, 100);

    // Create staging buffer for upload
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto staging = vw::create_buffer<StagingBuffer>(
        *gpu.allocator, 100 * sizeof(float));

    // Write to staging, then copy to GPU buffer via command buffer
    // ...
}
```

### Struct Buffer Testing

```cpp
TEST(BufferTest, StructBuffer) {
    struct Vertex {
        float x, y, z;
        float u, v;
    };

    auto &gpu = vw::tests::create_gpu();
    using VertexBuffer = vw::Buffer<Vertex, true, vw::VertexBufferUsage>;
    auto buffer = vw::create_buffer<VertexBuffer>(*gpu.allocator, 100);

    Vertex v{1.0f, 2.0f, 3.0f, 0.5f, 0.5f};
    buffer.write(v, 0);

    auto retrieved = buffer.read_as_vector(0, 1);
    EXPECT_FLOAT_EQ(retrieved[0].x, v.x);
    EXPECT_FLOAT_EQ(retrieved[0].y, v.y);
    EXPECT_FLOAT_EQ(retrieved[0].z, v.z);
}
```

---

## Test Organization

### Test Fixture Pattern

For tests sharing setup:

```cpp
class AllocatorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();

        cmdPool = std::make_unique<vw::CommandPool>(
            vw::CommandPoolBuilder(device).build());
    }

    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
    vw::Queue *queue;
    std::unique_ptr<vw::CommandPool> cmdPool;
};

TEST_F(AllocatorTest, CreateBuffer) {
    // Uses fixture members
    auto buffer = allocator->create_buffer(...);
}
```

### CMake Configuration

```cmake
# In VulkanWrapper/tests/CMakeLists.txt

add_executable(MemoryTests
    Memory/BufferTests.cpp
    Memory/IntervalTests.cpp
    Memory/AllocatorTests.cpp
)

target_link_libraries(MemoryTests
    PRIVATE
        TestUtils
        VulkanWrapperCoreLibrary
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(MemoryTests)
```

---

## Best Practices

### 1. Use Descriptive Test Names

```cpp
// Good
TEST(BufferIntervalSetTest, AddOverlappingIntervals_MergesIntoSingleInterval)

// Bad
TEST(BufferIntervalSetTest, Test1)
```

### 2. One Assertion Per Concept

```cpp
TEST(IntervalTest, MergePreservesCorrectBoundaries) {
    // Good: Multiple assertions for different aspects of the same concept
    EXPECT_EQ(merged.start, 0);
    EXPECT_EQ(merged.size, 150);
}
```

### 3. Use ASSERT for Critical Checks

```cpp
TEST(BufferTest, ReadBack) {
    auto data = buffer.read_as_vector(0, 10);

    // Use ASSERT when subsequent tests would crash without it
    ASSERT_EQ(data.size(), 10);

    // Now safe to access elements
    EXPECT_EQ(data[0], expected);
}
```

### 4. Clean Error Messages

```cpp
EXPECT_GT(luminance_lit, luminance_shadowed)
    << "Lit surface should be brighter"
    << " (lit=" << luminance_lit
    << ", shadowed=" << luminance_shadowed << ")";
```

### 5. Skip Tests When Features Unavailable

```cpp
TEST_F(RayTracingTest, BasicTracing) {
    if (!gpu) {
        GTEST_SKIP() << "Ray tracing not available on this system";
    }
    // Test code...
}
```

### 6. Avoid Test Interdependencies

Each test should be independent and runnable in isolation.

### 7. Use Tolerances for GPU Computations

```cpp
// GPU floating-point may have slight precision differences
constexpr float tolerance = 0.03f;
EXPECT_NEAR(result.r, expected.r, tolerance);
```

---

## See Also

- [RenderingTest.md](./RenderingTest.md) - Rendering test documentation
- [skill.md](./skill.md) - Testing skill overview
