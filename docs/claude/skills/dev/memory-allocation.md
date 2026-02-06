# Memory Allocation

Use `Allocator` for all memory. Never use raw Vulkan allocation APIs.

## Buffer Types

```cpp
// Template: Buffer<typename T, bool HostVisible, VkBufferUsageFlags Flags>
// Create via: create_buffer<T, HostVisible, UsageConstant>(allocator, count)
// Usage constants (VkBufferUsageFlags2 in BufferUsage.h):
//   UniformBufferUsage, StorageBufferUsage, StagingBufferUsage, VertexBufferUsage, IndexBufferUsage
```

**When to use host-visible vs staging:**
- `HostVisible = true`: small buffers updated frequently from CPU (uniforms, push-style data)
- `HostVisible = false` + `StagingBufferManager`: large, infrequently-updated data (vertex/index buffers, textures)

## Common Operations

```cpp
// Create allocator (buffer device address always enabled)
auto allocator = AllocatorBuilder(instance, device).build();

// Allocate buffers
auto vb = allocate_vertex_buffer<Vertex3D, false>(*allocator, count);
auto staging = create_buffer<std::byte, true, StagingBufferUsage>(*allocator, size);

// Type-alias style (recommended for complex usage flags)
using DeviceBuffer = Buffer<float, false, DeviceBufferUsage>;
auto buffer = create_buffer<DeviceBuffer>(*allocator, element_count);

// Create image
auto image = allocator->create_image_2D(Width{1920}, Height{1080}, false, format, usage);

// Host-visible buffer operations
buffer.write(data, offset);
auto result = buffer.read_as_vector(offset, count);
```

## Staging Uploads

Use `StagingBufferManager` to batch CPU-to-GPU transfers:

```cpp
StagingBufferManager staging(device, allocator);

// Queue multiple transfers (batched internally)
staging.fill_buffer(std::span<const float>(data), device_buffer, offset_in_elements);
staging.fill_buffer(std::span<const Vertex>(vertices), vertex_buffer, 0);

// Get recorded command buffer and submit
auto cmd = staging.fill_command_buffer();
queue.enqueue_command_buffer(cmd);
queue.submit({}, {}, {}).wait();
```

Also supports image loading:
```cpp
auto combined_image = staging.stage_image_from_path("texture.png", mipmaps);
```

## Random Sampling (for RT)

```cpp
auto samples = create_hemisphere_samples_buffer(*allocator);
NoiseTexture noise(device, allocator, queue);
```
