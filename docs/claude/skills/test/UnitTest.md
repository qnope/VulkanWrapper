# Unit Tests

GTest unit tests for GPU code.

## GPU Singleton

```cpp
auto& gpu = vw::tests::create_gpu();
// gpu.instance, gpu.device, gpu.allocator, gpu.queue()
```

For ray tracing:
```cpp
auto* gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```

## Common Patterns

**Buffer creation:**
```cpp
auto buffer = create_buffer<std::byte, true, StagingBufferUsage>(*gpu.allocator, 1024);
EXPECT_EQ(buffer.size(), 1024);
```

**Image creation:**
```cpp
auto image = gpu.allocator->create_image_2D(Width{64}, Height{64}, false, format, usage);
EXPECT_EQ(image->extent2D().width, 64);
```

**Command recording:**
```cpp
auto cmd = cmdPool.allocate(1)[0];
std::ignore = cmd.begin(vk::CommandBufferBeginInfo{}.setFlags(eOneTimeSubmit));
// record...
std::ignore = cmd.end();
gpu.queue().enqueue_command_buffer(cmd);
gpu.queue().submit({}, {}, {}).wait();
```

**Exception testing:**
```cpp
EXPECT_THROW(check_vk(vk::Result::eErrorUnknown, "test"), vw::VulkanException);
```

## Adding Tests

1. Create `VulkanWrapper/tests/MyTests.cpp`
2. Add to `CMakeLists.txt`:
```cmake
add_executable(MyTests MyTests.cpp)
target_link_libraries(MyTests PRIVATE TestUtils VulkanWrapperCoreLibrary GTest::gtest GTest::gtest_main)
gtest_discover_tests(MyTests)
```
