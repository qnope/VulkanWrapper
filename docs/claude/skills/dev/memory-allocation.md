# Memory Allocation

Always use the `Allocator` class for memory allocation. Never use raw Vulkan allocation APIs.

## Allocator Setup

```cpp
#include "VulkanWrapper/Memory/Allocator.h"

// Create allocator via builder
auto allocator = AllocatorBuilder(instance, device).build();

// With buffer device address support (for ray tracing)
auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/Allocator.h`

---

## Buffer Types

### Template Parameters

```cpp
Buffer<T, HostVisible, Usage>
// T:           Element type (uint32_t, Vertex3D, etc.)
// HostVisible: true = CPU-accessible, false = GPU-only
// Usage:       VkBufferUsageFlags (compile-time validated)
```

### Predefined Buffer Types

```cpp
// Index buffer (GPU-only)
using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

// Common usage flags
constexpr auto VertexBufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
constexpr auto IndexBufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
constexpr auto StorageBufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
constexpr auto UniformBufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h`

---

## Buffer Allocation

### Using AllocateBufferUtils

```cpp
#include "VulkanWrapper/Memory/AllocateBufferUtils.h"

// Vertex buffer (GPU-only)
auto vertexBuffer = allocate_vertex_buffer<Vertex3D, false>(*allocator, vertexCount);

// Vertex buffer (host-visible for dynamic updates)
auto dynamicVB = allocate_vertex_buffer<Vertex3D, true>(*allocator, vertexCount);

// Index buffer
auto indexBuffer = allocator->allocate_index_buffer(indexCount * sizeof(uint32_t));

// Generic buffer creation
auto storageBuffer = create_buffer<float, true, StorageBufferUsage>(*allocator, elementCount);
```

### Direct Allocation

```cpp
// Low-level allocation
BufferBase base = allocator->allocate_buffer(
    sizeInBytes,
    /*host_visible=*/false,
    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
    vk::SharingMode::eExclusive
);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/AllocateBufferUtils.h`

---

## Buffer Operations

### Host-Visible Buffers

```cpp
Buffer<float, true, StorageBufferUsage> buffer = ...;

// Write data
std::vector<float> data = {1.0f, 2.0f, 3.0f};
buffer.write(data, /*offset=*/0);

// Write single element
buffer.write(std::span(&value, 1), offset);

// Read data back
auto result = buffer.read_as_vector(/*offset=*/0, /*count=*/3);
```

### Compile-Time Validation

```cpp
// This compiles only if HostVisible == true
void write(std::span<const T> data, std::size_t offset)
    requires(HostVisible);

// Check usage at compile time
static consteval bool does_support(vk::BufferUsageFlags usage) {
    return (vk::BufferUsageFlags(flags) & usage) == usage;
}
```

---

## BufferList (Dynamic Pool)

For allocating many small buffers efficiently:

```cpp
#include "VulkanWrapper/Memory/BufferList.h"

BufferList<uint32_t, false, IndexBufferUsage> indexPool;

// Allocate with optional alignment
auto info = indexPool.create_buffer(indexCount, /*alignment=*/16);
// info.buffer - shared_ptr to the underlying buffer
// info.offset - offset within the buffer

// Use in draw call
cmd.bindIndexBuffer(info.buffer->handle(), info.offset * sizeof(uint32_t), vk::IndexType::eUint32);
```

### How It Works

- Allocates 16MB pages internally
- Finds existing page with space or creates new one
- Returns buffer + offset for sub-allocation
- Automatic alignment support

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/BufferList.h`

---

## UniformBufferAllocator

For dynamic uniform buffer allocation with proper alignment:

```cpp
#include "VulkanWrapper/Memory/UniformBufferAllocator.h"

// Create allocator with total size
UniformBufferAllocator uniformAllocator(allocator, /*totalSize=*/1024 * 1024);

// Allocate chunk for uniform data
struct MyUniforms {
    glm::mat4 viewProj;
    glm::vec4 lightDir;
};

auto chunk = uniformAllocator.allocate<MyUniforms>(1);
if (chunk) {
    // Use in descriptor
    vk::DescriptorBufferInfo bufferInfo{
        .buffer = chunk->handle,
        .offset = chunk->offset,
        .range = sizeof(MyUniforms)
    };

    // Write data (host-visible)
    // ...

    // Deallocate when done
    uniformAllocator.deallocate(chunk->index);
}
```

### Chunk Structure

```cpp
template <typename T>
struct UniformBufferChunk {
    vk::Buffer handle;           // Buffer handle
    vk::DeviceSize offset;       // Offset in buffer
    vk::DeviceSize size;         // Aligned size
    uint32_t index;              // For deallocation
    std::shared_ptr<...> buffer_ref;  // Keeps buffer alive
};
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/UniformBufferAllocator.h`

---

## Image Allocation

```cpp
// Create 2D image
auto image = allocator->create_image_2D(
    Width{1920},
    Height{1080},
    /*mipmap=*/true,  // Generate mipmaps
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
);

// Access properties
vk::Image handle = image->handle();
vk::ImageSubresourceRange range = image->full_range();
uint32_t mipLevels = image->mip_levels();
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/Allocator.h`

---

## Staging Buffer Pattern

For uploading data to GPU-only buffers:

```cpp
#include "VulkanWrapper/Memory/StagingBufferManager.h"

StagingBufferManager staging(allocator);

// Queue upload
staging.upload(deviceBuffer, data.data(), data.size() * sizeof(T), /*offset=*/0);

// Flush all pending uploads
staging.flush(cmd, queue);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/StagingBufferManager.h`

---

## Random Sampling Infrastructure

For ray tracing and Monte Carlo rendering, the project provides a centralized random sampling system.

### Hemisphere Samples Buffer

```cpp
#include "VulkanWrapper/Random/RandomSamplingBuffer.h"

// Create buffer with 4096 precomputed vec2 samples
auto samplesBuffer = create_hemisphere_samples_buffer(*allocator);

// Or with specific seed for reproducibility
auto samplesBuffer = create_hemisphere_samples_buffer(*allocator, 42);

// Generate samples manually
DualRandomSample samples = generate_hemisphere_samples();
// samples.samples[i] contains vec2 with values in [0, 1)
```

### Noise Texture

For per-pixel decorrelation via Cranley-Patterson rotation:

```cpp
#include "VulkanWrapper/Random/NoiseTexture.h"

// Create 4096x4096 RG32F noise texture
NoiseTexture noiseTexture(device, allocator, device->graphics_queue());

// Or with specific seed
NoiseTexture noiseTexture(device, allocator, device->graphics_queue(), 42);

// Use in descriptor binding
CombinedImage combined = noiseTexture.combined_image();
```

### GLSL Usage

```glsl
// Define bindings BEFORE including
#define RANDOM_XI_BUFFER_BINDING 8
#define RANDOM_NOISE_TEXTURE_BINDING 9
#include "random.glsl"

void main() {
    // Get decorrelated sample for this pixel
    vec2 xi = get_sample(sampleIndex, ivec2(gl_FragCoord.xy));

    // Cosine-weighted hemisphere direction
    vec3 dir = sample_hemisphere_cosine(normal, tangent, bitangent, xi);

    // PDF for importance sampling
    float pdf = pdf_hemisphere_cosine(dot(dir, normal));
}
```

### How It Works

1. **Precomputed samples**: 4096 low-discrepancy vec2 values stored in GPU buffer
2. **Noise texture**: 4096x4096 RG32F texture with random offsets
3. **Cranley-Patterson rotation**: `fract(sample + noise)` decorrelates neighboring pixels

This approach provides:
- Consistent low-discrepancy sampling
- Per-pixel variation without noise correlation
- Progressive refinement for path tracing

**References:**
- `VulkanWrapper/include/VulkanWrapper/Random/RandomSamplingBuffer.h`
- `VulkanWrapper/include/VulkanWrapper/Random/NoiseTexture.h`
- `VulkanWrapper/Shaders/include/random.glsl`

---

## Anti-Patterns

```cpp
// DON'T: Manual Vulkan allocation
VkBuffer buffer;
vkCreateBuffer(device, &createInfo, nullptr, &buffer);
VkDeviceMemory memory;
vkAllocateMemory(device, &allocInfo, nullptr, &memory);
vkBindBufferMemory(device, buffer, memory, 0);

// DO: Use Allocator
auto buffer = allocator->allocate_buffer(size, hostVisible, usage, sharingMode);

// DON'T: Raw pointers for buffer ownership
Buffer* buffer = new Buffer(...);

// DO: Smart pointers
auto buffer = std::make_shared<Buffer>(...);
```
