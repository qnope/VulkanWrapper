# Memory Allocation

Use `Allocator` for all memory. Never use raw Vulkan allocation APIs.

## Buffer Types

```cpp
// Template: Buffer<typename T, bool HostVisible, VkBufferUsageFlags Flags>
// Create via: create_buffer<T, HostVisible, UsageConstant>(allocator, count)
// Or type-alias: create_buffer<BufferType>(allocator, count)
// Factory functions in: Memory/AllocateBufferUtils.h
```

**Usage constants** (defined in `Memory/BufferUsage.h`):
| Constant | Includes | Use Case |
|----------|----------|----------|
| `VertexBufferUsage` | vertex + transferDst + deviceAddress + AS build input | Vertex data |
| `IndexBufferUsage` | index + transferDst + deviceAddress + AS build input | Index data |
| `UniformBufferUsage` | uniform + transferDst + deviceAddress | Uniform buffers |
| `StorageBufferUsage` | storage + transferDst + deviceAddress | SSBOs |
| `StagingBufferUsage` | transferSrc + transferDst + deviceAddress | CPU-GPU transfers |

**When to use host-visible vs staging:**
- `HostVisible = true`: small buffers updated frequently from CPU (uniforms, push-style data)
- `HostVisible = false` + `StagingBufferManager`: large, infrequently-updated data (vertex/index buffers, textures)

## Common Operations

```cpp
// Create allocator (buffer device address always enabled)
auto allocator = AllocatorBuilder(instance, device).build();

// Allocate buffers (AllocateBufferUtils.h)
auto vb = allocate_vertex_buffer<Vertex3D, false>(*allocator, count);
auto staging = create_buffer<std::byte, true, StagingBufferUsage>(*allocator, size);

// Type-alias style (recommended for complex usage flags)
using DeviceBuffer = Buffer<float, false, StorageBufferUsage>;
auto buffer = create_buffer<DeviceBuffer>(*allocator, element_count);

// Create image
auto image = allocator->create_image_2D(Width{1920}, Height{1080}, false, format, usage);

// Host-visible buffer operations (only when HostVisible = true, enforced by requires clause)
buffer.write(data, offset);         // std::span<const T>, offset in elements
auto result = buffer.read_as_vector(offset, count);

// Buffer properties
buffer.size_bytes();      // VkDeviceSize in bytes
buffer.device_address();  // vk::DeviceAddress for shader access
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

## Important Notes

- All usage constants include `eShaderDeviceAddress` (buffers accessible via device address in shaders)
- `VertexBufferUsage` and `IndexBufferUsage` include `eAccelerationStructureBuildInputReadOnlyKHR` for RT
- `buffer.write()` / `buffer.read_as_vector()` only available when `HostVisible = true` (enforced via `requires`)
- `Buffer::does_support(usage)` is `consteval` -- checks buffer usage flags at compile time
