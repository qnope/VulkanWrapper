# Unit Testing Guide

This document explains how to write unit tests for the VulkanWrapper project using Google Test (GTest).

## Test Framework

The project uses **Google Test (GTest)** for all unit testing. Tests are automatically discovered using CMake's `gtest_discover_tests()` function.

## GPU Singleton Pattern

Most tests require GPU access. Use the provided singleton pattern:

```cpp
#include "utils/create_gpu.hpp"

namespace vw::tests {

TEST(MyTest, BasicTest) {
    auto& gpu = create_gpu();

    // gpu.instance - Vulkan instance with validation layers
    // gpu.device - Logical device with graphics queue
    // gpu.allocator - VMA-based memory allocator
    // gpu.queue() - Graphics queue reference
}

} // namespace vw::tests
```

### GPU Singleton Implementation

The singleton is intentionally leaked to avoid static destruction order issues:

```cpp
GPU& create_gpu() {
    static GPU* gpu = []() {
        auto instance = InstanceBuilder()
            .setDebug()                        // Enable validation
            .setApiVersion(ApiVersion::e13)    // Vulkan 1.3
            .build();

        auto device = instance->findGpu()
            .with_queue(vk::QueueFlagBits::eGraphics)
            .with_synchronization_2()
            .with_dynamic_rendering()
            .with_descriptor_indexing()
            .build();

        auto allocator = AllocatorBuilder(instance, device).build();

        return new GPU{std::move(instance), std::move(device),
                       std::move(allocator)};
    }();
    return *gpu;
}
```

### Ray Tracing GPU

For tests requiring ray tracing, create a specialized GPU:

```cpp
struct RayTracingGPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    std::optional<Model::MeshManager> mesh_manager;

    Queue& queue() { return device->graphicsQueue(); }
};

RayTracingGPU* create_ray_tracing_gpu() {
    try {
        auto instance = InstanceBuilder()
            .setDebug()
            .setApiVersion(ApiVersion::e13)
            .build();

        auto device = instance->findGpu()
            .with_queue(vk::QueueFlagBits::eGraphics)
            .with_synchronization_2()
            .with_dynamic_rendering()
            .with_ray_tracing()              // Ray tracing extension
            .with_descriptor_indexing()
            .build();

        auto allocator = AllocatorBuilder(instance, device).build();
        return new RayTracingGPU{...};
    } catch (...) {
        return nullptr;  // Ray tracing not available
    }
}
```

## Test Fixtures

Use GTest fixtures for shared setup:

```cpp
class MyPassTest : public ::testing::Test {
protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) {
            GTEST_SKIP() << "Ray tracing not available";
        }

        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(gpu->device).build());
    }

    RayTracingGPU* gpu = nullptr;
    std::unique_ptr<CommandPool> cmdPool;
};
```

## Common Test Patterns

### Buffer Creation and Validation

```cpp
TEST(MemoryTests, BufferCreation) {
    auto& gpu = create_gpu();

    using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
    auto buffer = create_buffer<StagingBuffer>(*gpu.allocator, 1024);

    EXPECT_EQ(buffer.size(), 1024);
    EXPECT_TRUE(buffer.handle());
}
```

### Image Creation

```cpp
TEST(ImageTests, CreateImage2D) {
    auto& gpu = create_gpu();

    auto image = gpu.allocator->create_image_2D(
        Width{64}, Height{64}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eTransferSrc);

    EXPECT_EQ(image->extent2D().width, 64);
    EXPECT_EQ(image->extent2D().height, 64);
}
```

### Command Buffer Recording

```cpp
TEST(CommandTests, RecordAndSubmit) {
    auto& gpu = create_gpu();
    auto cmdPool = CommandPoolBuilder(gpu.device).build();
    auto cmd = cmdPool.allocate(1)[0];

    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Record commands...

    std::ignore = cmd.end();
    gpu.queue().enqueue_command_buffer(cmd);
    gpu.queue().submit({}, {}, {}).wait();
}
```

### Shader Compilation

```cpp
TEST(ShaderTests, CompileFromFile) {
    auto& gpu = create_gpu();
    auto shader_dir = std::filesystem::path(__FILE__)
        .parent_path().parent_path().parent_path() / "Shaders";

    ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);
    compiler.add_include_path(shader_dir / "include");

    auto shader = compiler.compile_file_to_module(
        gpu.device, shader_dir / "fullscreen.vert");

    ASSERT_NE(shader, nullptr);
}
```

## Resource Tracking and Barriers

Use `ResourceTracker` for automatic barrier generation:

```cpp
TEST(SyncTests, ResourceTracking) {
    auto& gpu = create_gpu();
    // ... create image ...

    Transfer transfer;
    auto& tracker = transfer.resourceTracker();

    // Track current state
    tracker.track(Barrier::ImageState{
        .image = image->handle(),
        .subresourceRange = image->full_range(),
        .layout = vk::ImageLayout::eUndefined,
        .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
        .access = vk::AccessFlagBits2::eNone
    });

    // Request new state
    tracker.request(Barrier::ImageState{
        .image = image->handle(),
        .subresourceRange = image->full_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite
    });

    // Generate barriers
    tracker.flush(cmd);
}
```

## Pixel Readback for Validation

### Reading RGBA8 Pixels

```cpp
auto pixels = stagingBuffer.read_as_vector(0, bufferSize);
EXPECT_EQ(static_cast<uint8_t>(pixels[0]), 255) << "R should be 255";
EXPECT_EQ(static_cast<uint8_t>(pixels[1]), 0)   << "G should be 0";
EXPECT_EQ(static_cast<uint8_t>(pixels[2]), 0)   << "B should be 0";
EXPECT_EQ(static_cast<uint8_t>(pixels[3]), 255) << "A should be 255";
```

### Reading Float16 (HDR) Pixels

```cpp
#include <glm/gtc/packing.hpp>

auto pixels = staging.read_as_vector(0, buffer_size);
const uint16_t* data = reinterpret_cast<const uint16_t*>(pixels.data());

size_t pixel_idx = (y * width + x) * 4;
glm::vec4 color(
    glm::unpackHalf1x16(data[pixel_idx]),
    glm::unpackHalf1x16(data[pixel_idx + 1]),
    glm::unpackHalf1x16(data[pixel_idx + 2]),
    glm::unpackHalf1x16(data[pixel_idx + 3])
);
```

### Reading Float32 Pixels

```cpp
auto pixels = staging.read_as_vector(0, buffer_size);
const float* data = reinterpret_cast<const float*>(pixels.data());

size_t pixel_idx = (y * width + x) * 4;
glm::vec4 color(
    data[pixel_idx],
    data[pixel_idx + 1],
    data[pixel_idx + 2],
    data[pixel_idx + 3]
);
```

## Adding New Tests

### 1. Create Test File

Create a new file in the appropriate subdirectory:

```cpp
// VulkanWrapper/tests/Memory/NewTests.cpp
#include "utils/create_gpu.hpp"
#include <gtest/gtest.h>

namespace vw::tests {

TEST(NewFeature, BasicTest) {
    auto& gpu = create_gpu();
    // Test implementation
}

} // namespace vw::tests
```

### 2. Update CMakeLists.txt

Add to `VulkanWrapper/tests/CMakeLists.txt`:

```cmake
add_executable(NewTests Memory/NewTests.cpp)
target_link_libraries(NewTests
    PRIVATE
        TestUtils
        VulkanWrapperCoreLibrary
        GTest::gtest
        GTest::gtest_main
)
gtest_discover_tests(NewTests)
```

### 3. Build and Run

```bash
cmake --build build-Clang20Debug --target NewTests
cd build-Clang20Debug/VulkanWrapper/tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./NewTests
```

## Test Filtering

Run specific tests using GTest's filter:

```bash
# Run tests matching a pattern
./RenderPassTests --gtest_filter='*SkyLight*'

# Run single test
./MemoryTests --gtest_filter='BufferTests.CreateStagingBuffer'

# Exclude tests
./VulkanTests --gtest_filter='*:-*Slow*'

# List all tests without running
./RenderPassTests --gtest_list_tests
```

## Testing Exceptions

Test that errors are properly thrown:

```cpp
TEST(ErrorHandling, InvalidBufferSizeThrows) {
    auto& gpu = create_gpu();

    EXPECT_THROW(
        create_buffer<StagingBuffer>(*gpu.allocator, 0),
        vw::LogicException
    );
}

TEST(ErrorHandling, InvalidShaderPathThrows) {
    auto& gpu = create_gpu();
    ShaderCompiler compiler;

    EXPECT_THROW(
        compiler.compile_file_to_module(gpu.device, "nonexistent.vert"),
        vw::FileException
    );
}
```

## Debugging Tests

### Using LLDB

```bash
cd build-Clang20Debug/VulkanWrapper/tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH lldb ./MemoryTests

# In LLDB:
(lldb) b BufferTests_CreateStagingBuffer_Test::TestBody
(lldb) run
(lldb) n  # step over
(lldb) p buffer.size()  # print value
```

### Validation Layer Output

Validation errors appear in stderr during test execution. Common issues:
- Missing image layout transitions
- Invalid descriptor bindings
- Synchronization hazards

Enable verbose validation:
```cpp
auto instance = InstanceBuilder()
    .setDebug()  // Enables validation layers
    .build();
```

## When to Use Fixtures vs Plain Tests

**Use plain `TEST()`** for:
- Simple, independent tests
- Tests that only need the basic GPU singleton

```cpp
TEST(BufferTests, CreateStagingBuffer) {
    auto& gpu = create_gpu();
    // Simple test
}
```

**Use `TEST_F()` with fixtures** for:
- Tests sharing setup/teardown logic
- Tests requiring specialized GPU configuration (e.g., ray tracing)
- Groups of related tests with common resources

```cpp
class RayTracingTest : public ::testing::Test {
protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
    }
    RayTracingGPU* gpu = nullptr;
};

TEST_F(RayTracingTest, BuildBLAS) { /* uses this->gpu */ }
TEST_F(RayTracingTest, BuildTLAS) { /* uses this->gpu */ }
```

## Best Practices

1. **Use Descriptive Test Names**
   ```cpp
   TEST_F(SunLightPassTest, ShadowOcclusion_LitVsShadowed)
   TEST_F(SkyLightPassTest, ZenithSun_ProducesBlueIndirectLight)
   ```

2. **Provide Clear Failure Messages**
   ```cpp
   EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)
       << "Lit surface should receive at least 2x more light"
       << " (lit=" << luminance_lit << ", shadowed=" << luminance_shadowed << ")";
   ```

3. **Skip Tests When Features Unavailable**
   ```cpp
   if (!gpu) {
       GTEST_SKIP() << "Ray tracing not available on this system";
   }
   ```

4. **Clean Up Resources Properly**
   - Use RAII for all Vulkan resources
   - Wait for GPU operations to complete before reading results
   ```cpp
   gpu.queue().submit({}, {}, {}).wait();
   ```

5. **Use Appropriate Tolerances**
   ```cpp
   constexpr float tolerance = 0.02f;
   EXPECT_NEAR(result.r, expected.r, tolerance);
   ```
