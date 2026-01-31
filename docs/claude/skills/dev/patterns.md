# Common Patterns

## Screen-Space Pass Pattern

For fullscreen post-processing effects:

```cpp
#include "VulkanWrapper/RenderPass/ScreenSpacePass.h"

// Define slots for lazy image allocation
enum class ToneMappingSlots { Output };

class ToneMappingPass : public ScreenSpacePass<ToneMappingSlots> {
public:
    ToneMappingPass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator)
        : ScreenSpacePass(std::move(device), std::move(allocator)) {
        m_sampler = create_default_sampler();
        create_pipeline();
        create_descriptor_pool();
    }

    void execute(vk::CommandBuffer cmd,
                 vk::Extent2D extent,
                 const std::shared_ptr<const Image>& hdrInput,
                 uint32_t frameIndex) {
        // Lazy allocation of output image
        auto& output = get_or_create_image(
            ToneMappingSlots::Output,
            extent.width, extent.height, frameIndex,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled
        );

        // Update descriptor with input
        update_descriptor(hdrInput);

        // Setup attachment
        vk::RenderingAttachmentInfo colorAttachment{
            .imageView = output.view->handle(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore
        };

        // Push constants
        struct PushConstants {
            float exposure;
            float gamma;
        } pc{1.0f, 2.2f};

        // Render fullscreen quad
        render_fullscreen(cmd, extent, colorAttachment, nullptr,
                          *m_pipeline, m_descriptorSet,
                          &pc, sizeof(pc));
    }

    const CachedImage& get_output(uint32_t frameIndex) const {
        return get_cached_image(ToneMappingSlots::Output, frameIndex);
    }

private:
    std::unique_ptr<Sampler> m_sampler;
    std::unique_ptr<Pipeline> m_pipeline;
    DescriptorSet m_descriptorSet;
};
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/RenderPass/ScreenSpacePass.h`

---

## Builder Pattern

All major objects use fluent builders:

```cpp
// Instance
auto instance = InstanceBuilder()
    .setDebug()
    .addPortability()  // For MoltenVK
    .setApiVersion(ApiVersion::e13)
    .build();

// Device
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// Command pool
auto commandPool = CommandPoolBuilder(device)
    .set_queue_family(device->graphics_queue().familyIndex())
    .set_flags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .build();

// Image view
auto imageView = ImageViewBuilder(device, image)
    .set_aspect(vk::ImageAspectFlagBits::eColor)
    .set_format(vk::Format::eR8G8B8A8Srgb)
    .build();

// Sampler
auto sampler = SamplerBuilder(device)
    .set_filter(vk::Filter::eLinear)
    .set_address_mode(vk::SamplerAddressMode::eRepeat)
    .set_anisotropy(16.0f)
    .build();

// Semaphore
auto semaphore = SemaphoreBuilder(device).build();
```

---

## RAII Handle Wrappers

### ObjectWithHandle (Non-Owning)

```cpp
// For handles that are borrowed or externally managed
class ObjectWithHandle<vk::Buffer> {
    vk::Buffer m_handle;
public:
    vk::Buffer handle() const noexcept { return m_handle; }
};
```

### ObjectWithUniqueHandle (Owning)

```cpp
// For handles with automatic destruction
class ObjectWithUniqueHandle<vk::UniqueImageView> {
    vk::UniqueImageView m_handle;
public:
    vk::ImageView handle() const noexcept { return *m_handle; }
    // Automatically destroyed when object is destroyed
};
```

---

## Error Handling

### Check Functions

```cpp
#include "VulkanWrapper/Utils/Error.h"

// Check vk::Result
check_vk(result, "create buffer");

// Check vk::ResultValue<T> - returns the value
auto buffer = check_vk(device.createBuffer(info), "create buffer");

// Check std::pair<vk::Result, T>
auto [result, value] = device.createSomething(info);
check_vk(result, "create something");

// Check VMA result
check_vma(vmaCreateBuffer(...), "allocate buffer");

// Check SDL
check_sdl(SDL_Init(SDL_INIT_VIDEO), "init SDL");
auto window = check_sdl(SDL_CreateWindow(...), "create window");
```

### Exception Hierarchy

```cpp
vw::Exception           // Base - message + std::source_location
├── VulkanException     // vk::Result errors
├── SDLException        // SDL errors (auto-captures SDL_GetError())
├── VMAException        // VMA allocation errors
├── FileException       // File I/O errors
├── AssimpException     // Model loading errors
├── ShaderCompilationException
└── LogicException      // Logic/state errors
    ├── out_of_range
    ├── invalid_state
    └── null_pointer
```

### Exception Usage

```cpp
// Catch specific exception
try {
    auto buffer = allocator->allocate_buffer(...);
} catch (const vw::VMAException& e) {
    std::cerr << "Allocation failed: " << e.what()
              << " at " << e.location().file_name()
              << ":" << e.location().line() << "\n";
}

// Throw specific exception
throw vw::LogicException(vw::LogicException::Kind::invalid_state,
                         "pipeline not bound");
throw vw::FileException("config.json", "file not found");
```

---

## Command Buffer Recording

```cpp
#include "VulkanWrapper/Command/CommandBufferRecorder.h"

auto commandPool = CommandPoolBuilder(device).build();
auto commandBuffers = commandPool.allocate(frameCount);

// RAII recording - begins in constructor, ends in destructor
{
    CommandBufferRecorder recorder(commandBuffers[frameIndex]);

    // Record commands
    tracker.flush(recorder.handle());
    recorder.handle().beginRendering(renderingInfo);
    recorder.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    recorder.handle().draw(3, 1, 0, 0);
    recorder.handle().endRendering();
}  // Recording ends here

// Submit
queue.submit(submitInfo);
```

---

## Descriptor Management

### Layout Creation

```cpp
auto layout = DescriptorSetLayoutBuilder(device)
    .add_binding(0, vk::DescriptorType::eUniformBuffer,
                 vk::ShaderStageFlagBits::eVertex)
    .add_binding(1, vk::DescriptorType::eCombinedImageSampler,
                 vk::ShaderStageFlagBits::eFragment)
    .build();
```

### Pool and Allocation

```cpp
auto pool = DescriptorPoolBuilder(device)
    .set_max_sets(100)
    .add_pool_size(vk::DescriptorType::eUniformBuffer, 100)
    .add_pool_size(vk::DescriptorType::eCombinedImageSampler, 100)
    .build();

auto descriptorSet = pool.allocate(layout);
```

### Updating Descriptors

```cpp
vk::DescriptorBufferInfo bufferInfo{
    .buffer = uniformBuffer.handle(),
    .offset = 0,
    .range = sizeof(UniformData)
};

vk::DescriptorImageInfo imageInfo{
    .sampler = sampler.handle(),
    .imageView = textureView.handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};

std::array writes = {
    vk::WriteDescriptorSet{
        .dstSet = descriptorSet.handle(),
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &bufferInfo
    },
    vk::WriteDescriptorSet{
        .dstSet = descriptorSet.handle(),
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo
    }
};

device->handle().updateDescriptorSets(writes, {});
```

---

## Code Style Quick Reference

| Element | Convention | Example |
|---------|------------|---------|
| Functions | `snake_case` | `create_buffer()` |
| Types | `PascalCase` | `BufferList` |
| Members | `m_snake_case` | `m_device` |
| Constants | `PascalCase` | `VertexBufferUsage` |
| Namespaces | `lowercase` | `vw::Barrier` |
| Files | `PascalCase.h/.cpp` | `ResourceTracker.h` |
| Enums | `PascalCase` values | `enum class Slots { Output }` |

### Strong Types for Dimensions

```cpp
// Always use strong types for dimensions
auto image = allocator->create_image_2D(
    Width{1920},   // Not just 1920
    Height{1080},  // Not just 1080
    ...
);
```

### Ownership Conventions

```cpp
std::shared_ptr<const Device> m_device;  // Shared, immutable
std::shared_ptr<Allocator> m_allocator;  // Shared, mutable
std::unique_ptr<Pipeline> m_pipeline;    // Exclusive ownership
vk::Buffer m_handle;                     // Non-owning (borrowed)
```

### Header Organization

```cpp
#pragma once
#include "VulkanWrapper/3rd_party.h"  // Always first (precompiled header)
#include "VulkanWrapper/fwd.h"        // Forward declarations
#include <standard_headers>            // Standard library
#include <external_headers>            // Third-party
```

---

## Troubleshooting

### Validation Layer Errors

**Enable validation layers:**
```cpp
auto instance = InstanceBuilder()
    .setDebug()  // Enables VK_LAYER_KHRONOS_validation
    .build();
```

**Common errors and solutions:**

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `VUID-vkCmdDraw-None-02699` | Pipeline not bound | Call `cmd.bindPipeline()` before draw |
| `VUID-VkRenderingInfo-...` | Missing attachment | Check all required attachments are set |
| `VUID-vkQueueSubmit-pCommandBuffers-...` | Command buffer state | Ensure recording ended properly |
| `VUID-vkCmdPipelineBarrier2-...` | Invalid barrier | Check stage/access flags match usage |

### Synchronization Issues

**Symptoms:**
- Flickering/artifacts
- Missing geometry
- Crashes only on some GPUs

**Diagnosis:**
```cpp
// Use validation's synchronization validation
// Set VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
```

**Common mistakes:**

```cpp
// DON'T: Use image without transition
tracker.request(state);
// tracker.flush(cmd);  <- Missing!
cmd.beginRendering(...);  // Race condition

// DO: Always flush before use
tracker.request(state);
tracker.flush(cmd);
cmd.beginRendering(...);

// DON'T: Wrong stage/access combination
tracker.request(ImageState{
    .layout = eShaderReadOnlyOptimal,
    .stage = eColorAttachmentOutput,  // Wrong stage!
    .access = eShaderSampledRead
});

// DO: Match stage and access
tracker.request(ImageState{
    .layout = eShaderReadOnlyOptimal,
    .stage = eFragmentShader,  // Correct stage
    .access = eShaderSampledRead
});
```

### Memory Allocation Failures

**VMA errors:**
```cpp
try {
    auto buffer = allocator->allocate_buffer(size, hostVisible, usage, sharingMode);
} catch (const vw::VMAException& e) {
    // e.result() contains VkResult
    // Common: VK_ERROR_OUT_OF_DEVICE_MEMORY
}
```

**Solutions:**
- Reduce allocation size
- Use GPU-only buffers when possible (`HostVisible = false`)
- Check `VkPhysicalDeviceMemoryProperties` for available heaps
- Consider `BufferList` for many small allocations

### Shader Compilation Errors

**Runtime compilation:**
```cpp
try {
    auto module = compiler.compile_file_to_module(device, "shader.frag");
} catch (const vw::ShaderCompilationException& e) {
    // e.what() contains full error message with line numbers
    std::cerr << e.what() << "\n";
}
```

**Common issues:**

| Error | Cause | Solution |
|-------|-------|----------|
| `#include not found` | Missing include path | Add path via `add_include_path()` |
| `undefined symbol` | Missing extension | Add `#extension GL_... : require` |
| `type mismatch` | Layout incompatibility | Check C++/GLSL struct alignment |

### Ray Tracing Issues

**Build failures:**
```cpp
// Ensure buffer device address is enabled
auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

**Empty output:**
- Check TLAS is built before tracing
- Verify barriers between AS build and trace
- Ensure hit shaders write to output

### Debugging Tips

1. **Use RenderDoc** for frame capture and analysis
2. **Enable VK_LAYER_LUNARG_api_dump** for call logging
3. **Use `device->wait_idle()`** before reading back data
4. **Check exception locations** via `e.location().file_name()` and `e.location().line()`
5. **Simplify** - isolate the failing operation in a minimal test
