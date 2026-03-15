---
sidebar_position: 3
---

# Vulkan Primer

This page introduces the core concepts of Vulkan for developers who are new to GPU programming or are coming from higher-level APIs like OpenGL. Each concept is explained in plain terms, followed by how VulkanWrapper maps to it.

If you already know Vulkan, you can skip ahead to the [VulkanWrapper Mapping](#how-vulkanwrapper-maps-to-vulkan) section or go directly to the [Getting Started](./getting-started) guide.

## What is Vulkan?

Vulkan is a low-level graphics and compute API maintained by the Khronos Group. It gives you explicit control over the GPU: you decide when memory is allocated, when commands are submitted, and how different operations synchronize with each other.

This explicitness is what makes Vulkan fast -- the driver does not guess your intentions or insert hidden work behind your back. But it also means you are responsible for correctness: mistakes that OpenGL would silently handle can cause crashes, visual corruption, or data races in Vulkan.

VulkanWrapper sits on top of Vulkan. It handles the tedious and error-prone parts (resource lifetime, synchronization barriers, verbose object creation) while keeping you in direct contact with the Vulkan execution model.

## Key Concepts

### Instance

The **Instance** is the entry point to Vulkan. It represents your application's connection to the Vulkan runtime. When you create an instance, you specify:

- The Vulkan **API version** you require (VulkanWrapper targets 1.3).
- Which **instance extensions** you need (for example, window surface support).
- Whether to enable **validation layers** for debugging.

An application typically creates exactly one Instance.

**In VulkanWrapper:**

```cpp
auto instance = vw::InstanceBuilder()
    .setApiVersion(vw::ApiVersion::e13)
    .setDebug()  // Enable validation layers
    .addPortability()  // Required on macOS
    .addExtensions(window.get_required_instance_extensions())
    .build();
```

The `InstanceBuilder` handles the verbose `VkInstanceCreateInfo` setup and validation layer configuration for you.

### Physical Device

A **Physical Device** represents an actual GPU installed in the system (or a software renderer). Your machine might have multiple physical devices -- a discrete GPU, an integrated GPU, and possibly a software fallback.

Before you can use a GPU, you need to query its capabilities: which queue families it supports, what extensions it offers, how much memory it has, and so on.

**In VulkanWrapper:**

You do not interact with physical devices directly. The `DeviceFinder` (returned by `instance->findGpu()`) scans all available GPUs and selects one that meets your requirements:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();
```

If no GPU meets the requirements, `build()` throws an exception.

### Logical Device

The **Logical Device** (usually just "Device") is your application's interface to a specific physical GPU. It is the object from which you create all other Vulkan resources: buffers, images, pipelines, command pools, and so on.

Creating a logical device also creates the **queues** you requested.

**In VulkanWrapper:**

The `DeviceFinder::build()` method returns a `std::shared_ptr<Device>`. Most builder classes take this shared pointer as a parameter:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// Access the underlying Vulkan handle when needed
vk::Device vulkanDevice = device->handle();
vk::PhysicalDevice physicalDevice = device->physical_device();
```

### Queues

A **Queue** is a channel through which you submit work (command buffers) to the GPU. GPUs organize queues into **queue families**, each supporting different operation types:

- **Graphics** queues can run vertex shading, fragment shading, and rendering commands.
- **Compute** queues can run compute shaders.
- **Transfer** queues can copy data between buffers and images.

Many GPUs have a single queue family that supports all three types. Some have dedicated transfer or compute queues for async work.

**In VulkanWrapper:**

```cpp
// Request a queue that supports graphics, compute, and transfer
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics |
                vk::QueueFlagBits::eCompute |
                vk::QueueFlagBits::eTransfer)
    .build();

// Access the queue
vw::Queue &queue = device->graphicsQueue();
```

### Command Buffers

A **Command Buffer** is a recorded list of GPU commands. You do not call GPU functions directly -- instead, you record commands into a command buffer, then submit the entire buffer to a queue for execution.

This two-step model is what allows Vulkan to be efficient: the driver can optimize the entire command stream at once, and you can record command buffers on multiple CPU threads in parallel.

The lifecycle is:
1. **Allocate** a command buffer from a command pool.
2. **Begin** recording.
3. **Record** commands (draw calls, dispatches, copies, barriers).
4. **End** recording.
5. **Submit** to a queue.

**In VulkanWrapper:**

```cpp
auto commandPool = vw::CommandPoolBuilder(device)
    .with_reset_command_buffer()
    .build();

auto commandBuffers = commandPool.allocate(1);
auto cmd = commandBuffers[0];

cmd.begin(vk::CommandBufferBeginInfo{});
// Record commands...
cmd.end();
```

### Synchronization

Vulkan gives you zero implicit synchronization. If you write to a buffer on the GPU and then read from it in a subsequent draw call, you must explicitly tell the GPU to wait for the write to finish before starting the read. There are three synchronization mechanisms:

#### Fences

A **Fence** synchronizes the CPU with the GPU. You submit work and then wait on a fence to know when the GPU has finished.

#### Semaphores

A **Semaphore** synchronizes different queue submissions on the GPU. For example, you can use a semaphore to ensure rendering finishes before presenting to the screen.

#### Pipeline Barriers

A **Pipeline Barrier** synchronizes operations within a single command buffer (or across queue submissions). Barriers specify:

- The **source stage** and **access** (what must finish).
- The **destination stage** and **access** (what is waiting).
- For images: the **layout transition** (e.g., from "undefined" to "color attachment").

Barriers are the most common synchronization primitive and the most error-prone. Getting them wrong can cause visual glitches, GPU hangs, or data corruption.

**In VulkanWrapper:**

The `ResourceTracker` eliminates manual barrier management. You tell it the current state of each resource and what state you need. It generates the optimal barriers:

```cpp
vw::Barrier::ResourceTracker tracker;

// Track initial state
tracker.track(vw::Barrier::ImageState{
    .image = depthImage,
    .subresourceRange = depthRange,
    .layout = vk::ImageLayout::eUndefined,
    .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});

// Request transition for use as depth attachment
tracker.request(vw::Barrier::ImageState{
    .image = depthImage,
    .subresourceRange = depthRange,
    .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
    .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
              vk::AccessFlagBits2::eDepthStencilAttachmentWrite
});

// The correct barrier is generated and recorded
tracker.flush(cmd);
```

The tracker also handles **buffer** and **acceleration structure** barriers:

```cpp
tracker.track(vw::Barrier::BufferState{
    .buffer = vertexBuffer,
    .offset = 0,
    .size = bufferSize,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});

tracker.request(vw::Barrier::BufferState{
    .buffer = vertexBuffer,
    .offset = 0,
    .size = bufferSize,
    .stage = vk::PipelineStageFlagBits2::eVertexInput,
    .access = vk::AccessFlagBits2::eVertexAttributeRead
});

tracker.flush(cmd);
```

### Pipeline

A **Graphics Pipeline** defines the entire configuration for rendering: which shaders to run, what vertex format to expect, how to blend colors, whether to perform depth testing, and more. In Vulkan, pipelines are immutable once created -- you cannot change individual settings, you must create a new pipeline.

This immutability is deliberate: it allows the driver to compile the pipeline into optimized GPU machine code up front, avoiding runtime compilation stalls.

**In VulkanWrapper:**

```cpp
auto pipeline = vw::GraphicsPipelineBuilder(device, pipelineLayout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
    .add_vertex_binding<Vertex>()
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)
    .set_depth_format(vk::Format::eD32Sfloat)
    .with_depth_test(true, vk::CompareOp::eLess)
    .with_topology(vk::PrimitiveTopology::eTriangleList)
    .with_cull_mode(vk::CullModeFlagBits::eBack)
    .with_dynamic_viewport_scissor()
    .build();
```

The `with_dynamic_viewport_scissor()` method marks the viewport and scissor as dynamic state, meaning you can change them per-frame without creating a new pipeline.

### Descriptors

**Descriptors** are how shaders access resources. A shader cannot directly reference a buffer or image -- it reads from descriptor bindings. You can think of descriptors as a table of pointers that the shader indexes into.

The descriptor system has three layers:

1. **Descriptor Set Layout** -- defines the shape of the table (which binding holds a uniform buffer, which holds a sampled image, etc.).
2. **Descriptor Set** -- an actual table filled with specific resource handles.
3. **Descriptor Pool** -- a memory pool from which descriptor sets are allocated.

**In VulkanWrapper:**

```cpp
auto layout = vw::DescriptorSetLayoutBuilder(device)
    .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
    .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
    .build();
```

### Images and Image Views

An **Image** is a multi-dimensional array of texels stored in GPU memory. Images can be 1D, 2D, 3D, or cube maps, and can have multiple mip levels and array layers.

An **Image View** describes how to interpret an image for a specific use. The same image could have a view that sees it as an `R8G8B8A8_SRGB` 2D texture and another view that sees a single mip level as a storage image.

**In VulkanWrapper:**

```cpp
// Create a 2D image
auto image = allocator->create_image_2D(
    vw::Width{1024}, vw::Height{1024},
    true,  // Generate mipmaps
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eTransferDst);

// Create a view of the image
auto view = vw::ImageViewBuilder(device, image).build();

// Create a sampler for texture filtering
auto sampler = vw::SamplerBuilder(device).build();
```

### Swapchain

The **Swapchain** is a queue of images that are presented to the screen. It typically contains 2 or 3 images (double or triple buffering). Your application renders to one image while the display hardware scans out another.

The swapchain is tied to a specific **Surface**, which represents the window's drawable area.

**In VulkanWrapper:**

```cpp
// Create a surface from the window
auto surface = window.create_surface(*instance);

// Request presentation support when finding the GPU
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_presentation(surface.handle())
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// Create the swapchain
auto swapchain = window.create_swapchain(device, surface.handle());
```

### Dynamic Rendering

Traditional Vulkan uses **Render Passes** and **Framebuffers** to describe the structure of a rendering operation. These are complex to set up and inflexible.

Vulkan 1.3 introduced **Dynamic Rendering** (`VK_KHR_dynamic_rendering`), which eliminates render pass objects entirely. Instead, you simply call `beginRendering()` with the color and depth attachments you want, record your draw calls, and call `endRendering()`.

VulkanWrapper uses dynamic rendering exclusively. You will never see `VkRenderPass`, `VkFramebuffer`, or `beginRenderPass` in this library.

**In practice:**

```cpp
vk::RenderingAttachmentInfo colorAttachment{};
colorAttachment.imageView = colorView;
colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
colorAttachment.clearValue = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};

vk::RenderingInfo renderingInfo{};
renderingInfo.renderArea = vk::Rect2D{{0, 0}, {width, height}};
renderingInfo.layerCount = 1;
renderingInfo.colorAttachmentCount = 1;
renderingInfo.pColorAttachments = &colorAttachment;

cmd.beginRendering(renderingInfo);
// Draw calls go here
cmd.endRendering();
```

## Synchronization2

Vulkan 1.3 promoted the **Synchronization2** extension to core. This is a modernized barrier model that replaces the original pipeline stage and access flag types with more expressive 64-bit versions:

| Old (do NOT use) | New (Synchronization2) |
|-------------------|----------------------|
| `vk::PipelineStageFlagBits` | `vk::PipelineStageFlagBits2` |
| `vk::AccessFlagBits` | `vk::AccessFlagBits2` / `vk::AccessFlags2` |
| `vkCmdPipelineBarrier` | `vkCmdPipelineBarrier2` |

The Synchronization2 model is clearer, more fine-grained, and less error-prone. VulkanWrapper uses it throughout. The `DeviceFinder` enables it with:

```cpp
.with_synchronization_2()
```

The `ResourceTracker` generates `vkCmdPipelineBarrier2` calls using `vk::PipelineStageFlagBits2` and `vk::AccessFlags2` exclusively.

## Ray Tracing Concepts

Vulkan supports hardware-accelerated ray tracing through a set of KHR extensions. VulkanWrapper provides builders and RAII wrappers for the key objects.

### What is Ray Tracing?

In rasterization (the traditional rendering approach), you project triangles onto the screen and shade the visible pixels. This is fast but struggles with effects that require knowledge of the entire scene: reflections, soft shadows, ambient occlusion, and global illumination.

Ray tracing works differently: for each pixel, you cast a ray from the camera into the scene and find what it hits. You can then cast secondary rays to simulate light bouncing, shadow testing, and more. This naturally handles complex lighting effects but is computationally expensive.

Modern GPUs have dedicated ray tracing hardware (RT cores) that accelerate the ray-scene intersection test.

### Acceleration Structures

To make ray tracing fast, the scene geometry is organized into an **Acceleration Structure** -- a spatial data structure (typically a bounding volume hierarchy) that allows the GPU to quickly determine which triangles a ray might intersect.

There are two levels:

#### BLAS (Bottom-Level Acceleration Structure)

A BLAS holds the actual triangle geometry for a single mesh. Each BLAS is built from vertex and index buffers.

**In VulkanWrapper** (namespace `vw::rt::as`):

```cpp
auto &blas = vw::rt::as::BottomLevelAccelerationStructureBuilder(device)
    .add_mesh(mesh)
    .build_into(blasList);
```

#### TLAS (Top-Level Acceleration Structure)

A TLAS holds **instances** of BLASes, each with a transform matrix. This is what lets you place the same mesh at multiple positions in the scene without duplicating geometry.

**In VulkanWrapper** (namespace `vw::rt::as`):

```cpp
auto tlas = vw::rt::as::TopLevelAccelerationStructureBuilder(device, allocator)
    .add_bottom_level_acceleration_structure_address(
        blas.device_address(), transform)
    .build(commandBuffer);
```

### Enabling Ray Tracing

To use ray tracing, request it when creating the device:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .with_ray_tracing()          // Enables acceleration structure and RT pipeline extensions
    .with_descriptor_indexing()  // Often needed for bindless resources in RT shaders
    .with_scalar_block_layout()  // Allows flexible buffer layouts in RT shaders
    .build();
```

Not all GPUs support ray tracing. On systems without hardware RT, the `build()` call will fail if no device meets the requirements.

## How VulkanWrapper Maps to Vulkan

Here is a summary of how VulkanWrapper classes correspond to Vulkan concepts:

| Vulkan Concept | VulkanWrapper Class | Created Via |
|----------------|-------------------|-------------|
| `VkInstance` | `vw::Instance` | `vw::InstanceBuilder` |
| `VkPhysicalDevice` | (internal to `DeviceFinder`) | `instance->findGpu()` |
| `VkDevice` | `vw::Device` | `DeviceFinder::build()` |
| `VkQueue` | `vw::Queue`, `vw::PresentQueue` | `device->graphicsQueue()` |
| `VkCommandPool` | `vw::CommandPool` | `vw::CommandPoolBuilder` |
| `VkBuffer` | `vw::Buffer<T, HostVisible, Flags>` | `vw::create_buffer<>()` |
| `VkImage` | `vw::Image` | `allocator->create_image_2D()` |
| `VkImageView` | `vw::ImageView` | `vw::ImageViewBuilder` |
| `VkSampler` | `vw::Sampler` | `vw::SamplerBuilder` |
| `VkPipeline` (graphics) | `vw::Pipeline` | `vw::GraphicsPipelineBuilder` |
| `VkPipelineLayout` | `vw::PipelineLayout` | `vw::PipelineLayoutBuilder` |
| `VkDescriptorSetLayout` | `vw::DescriptorSetLayout` | `vw::DescriptorSetLayoutBuilder` |
| `VkSwapchainKHR` | `vw::Swapchain` | `window.create_swapchain()` |
| `VkSurfaceKHR` | `vw::Surface` | `window.create_surface()` |
| `VkFence` | `vw::Fence` | `vw::FenceBuilder` |
| `VkSemaphore` | `vw::Semaphore` | `vw::SemaphoreBuilder` |
| Pipeline barriers | `vw::Barrier::ResourceTracker` | Direct construction |
| `VkAccelerationStructureKHR` (bottom) | `vw::rt::as::BottomLevelAccelerationStructure` | Builder + `build_into()` |
| `VkAccelerationStructureKHR` (top) | `vw::rt::as::TopLevelAccelerationStructure` | Builder + `build()` |
| VMA (`VmaAllocator`) | `vw::Allocator` | `vw::AllocatorBuilder` |
| Shader module | `vw::ShaderModule` | `vw::ShaderCompiler` |
| Image copy/blit | `vw::Transfer` | Direct construction |

## Glossary

- **API version**: The Vulkan specification version (1.0, 1.1, 1.2, 1.3). Higher versions include more features as core. VulkanWrapper requires 1.3.
- **Barrier**: A synchronization command that ensures one GPU operation completes before another begins. Also performs image layout transitions.
- **Bindless**: A technique where shaders access resources through indices into large descriptor arrays, rather than fixed bindings. Requires descriptor indexing.
- **BLAS**: Bottom-Level Acceleration Structure. Holds triangle geometry for ray tracing.
- **Command buffer**: A recorded list of GPU commands that is submitted to a queue for execution.
- **Descriptor**: A handle through which a shader accesses a buffer, image, or sampler.
- **Descriptor set**: A collection of descriptors bound together.
- **Device**: The logical connection to a GPU, from which all resources are created.
- **Dynamic rendering**: A Vulkan 1.3 feature that replaces render pass objects with simple begin/end rendering commands.
- **Fence**: A CPU-GPU synchronization primitive. The CPU can wait on a fence to know when submitted GPU work has completed.
- **Framebuffer**: A legacy Vulkan concept (collection of image views for a render pass). NOT used in VulkanWrapper -- replaced by dynamic rendering.
- **GLSL**: OpenGL Shading Language. The high-level shader language compiled to SPIR-V for Vulkan.
- **Host-visible**: A memory property indicating the CPU can read/write the memory directly (mapped memory).
- **Image**: A multi-dimensional texel array in GPU memory (1D, 2D, 3D, or cube).
- **Image layout**: The internal memory arrangement of an image, optimized for specific uses (e.g., color attachment, shader reading, transfer source).
- **Image view**: A description of how to interpret an image (format, aspect, mip range, array range).
- **Instance**: The application's connection to the Vulkan runtime. Entry point for all Vulkan operations.
- **Mipmap**: A chain of progressively smaller versions of an image, used for texture filtering at different distances.
- **Physical device**: An actual GPU (or software renderer) in the system.
- **Pipeline**: An immutable, compiled configuration for rendering or compute. Defines shaders, vertex format, blend state, depth testing, and more.
- **Pipeline layout**: Describes the descriptor set layouts and push constant ranges a pipeline uses.
- **Present queue**: A queue that can present rendered images to a window surface.
- **Push constants**: Small amounts of data (up to 128 or 256 bytes depending on hardware) passed directly to shaders without a descriptor set.
- **Queue**: A channel for submitting command buffers to the GPU.
- **Queue family**: A group of queues with the same capabilities (graphics, compute, transfer).
- **RAII**: Resource Acquisition Is Initialization. A C++ pattern where resource cleanup happens automatically in destructors.
- **Render pass**: A legacy Vulkan concept that describes the structure of rendering (attachments, subpasses, dependencies). NOT used in VulkanWrapper -- replaced by dynamic rendering.
- **Sampler**: An object that controls how textures are filtered and addressed (linear, nearest, repeat, clamp).
- **Semaphore**: A GPU-GPU synchronization primitive. Ensures one queue submission completes before another begins.
- **SPIR-V**: Standard Portable Intermediate Representation for Vulkan. The binary shader format consumed by Vulkan.
- **Staging buffer**: A host-visible buffer used as an intermediary to upload data to device-local (GPU-only) memory.
- **Subresource range**: A specification of which mip levels and array layers of an image to operate on.
- **Surface**: A platform-specific object representing a window's drawable area that Vulkan can present to.
- **Swapchain**: A queue of presentable images associated with a surface (double or triple buffering).
- **Synchronization2**: The modern (Vulkan 1.3 core) barrier model with 64-bit stage and access flags.
- **TLAS**: Top-Level Acceleration Structure. References BLAS instances with transforms for ray tracing.
- **Transfer**: An operation that copies data between buffers and/or images.
- **Validation layers**: Debug layers that check every Vulkan call for correctness and report errors. Essential during development, disabled in release builds.
- **VMA**: Vulkan Memory Allocator. A library by AMD that manages GPU memory allocation efficiently.

## Next Steps

- **[Getting Started](./getting-started)** -- build the project and run your first example.
- **[Core Concepts](./category/core-concepts)** -- deep dives into builders, RAII, type safety, and error handling.
- **[API Reference](./category/api-reference)** -- detailed documentation for each module.
