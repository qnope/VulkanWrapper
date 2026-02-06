# Unit Tests

GTest unit tests for GPU code. Test resource creation, memory management, APIs, and error handling.

## GPU Singleton

```cpp
#include "utils/create_gpu.hpp"
auto& gpu = vw::tests::create_gpu();  // Shared singleton
// gpu.instance  -- shared_ptr<Instance>
// gpu.device    -- shared_ptr<Device>
// gpu.allocator -- shared_ptr<Allocator>
// gpu.queue()   -- Queue&
```

For ray tracing, define `get_ray_tracing_gpu()` locally in your test file (not in a shared header):
```cpp
auto* gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```

## Examples and Common Patterns

**Buffer creation and validation:**
```cpp
auto buffer = create_buffer<std::byte, true, StagingBufferUsage>(*gpu.allocator, 1024);
EXPECT_EQ(buffer.size(), 1024);
EXPECT_NE(buffer.device_address(), 0);
```

**Image creation and validation:**
```cpp
auto image = gpu.allocator->create_image_2D(Width{64}, Height{64}, false, format, usage);
EXPECT_EQ(image->extent2D().width, 64);
```

**Command recording and submission:**
```cpp
auto cmdPool = CommandPoolBuilder(gpu.device).build();
auto cmd = cmdPool.allocate(1)[0];
std::ignore = cmd.begin(vk::CommandBufferBeginInfo{}.setFlags(eOneTimeSubmit));
// record commands...
std::ignore = cmd.end();
gpu.queue().enqueue_command_buffer(cmd);
gpu.queue().submit({}, {}, {}).wait();  // Always wait for GPU
```

**Exception testing:**
```cpp
EXPECT_THROW(check_vk(vk::Result::eErrorUnknown, "test"), vw::VulkanException);
EXPECT_THROW(check_vma(VK_ERROR_OUT_OF_DEVICE_MEMORY, "test"), vw::VMAException);
EXPECT_NO_THROW(check_vk(vk::Result::eSuccess, "ok"));
```

**ResourceTracker testing:**
```cpp
vw::Barrier::ResourceTracker tracker;
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
tracker.flush(cmd);
// Verify no validation errors after flush
```

**Staging buffer upload and readback:**
```cpp
StagingBufferManager staging(gpu.device, gpu.allocator);
staging.fill_buffer(std::span<const float>(data), device_buffer, 0);
auto cmd = staging.fill_command_buffer();
gpu.queue().enqueue_command_buffer(cmd);
gpu.queue().submit({}, {}, {}).wait();
// Readback to verify
auto result = staging_buffer.read_as_vector(0, count);
```

## Important Notes

- The GPU singleton intentionally leaks to avoid static destruction order issues -- this is expected
- Always call `.wait()` after `queue().submit()` to ensure GPU work completes before assertions
- Validation layers are enabled by default -- any Vulkan misuse will trigger validation errors
