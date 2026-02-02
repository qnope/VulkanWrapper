# Memory Allocation

Use `Allocator` for all memory. Never use raw Vulkan allocation APIs.

## Buffer Types

```cpp
Buffer<T, HostVisible, Usage>
// T: Element type
// HostVisible: true = CPU-accessible, false = GPU-only
// Usage: VkBufferUsageFlags
```

## Common Operations

```cpp
// Create allocator
auto allocator = AllocatorBuilder(instance, device).build();

// Allocate buffers
auto vb = allocate_vertex_buffer<Vertex3D, false>(*allocator, count);
auto staging = create_buffer<std::byte, true, StagingBufferUsage>(*allocator, size);

// Create image
auto image = allocator->create_image_2D(Width{1920}, Height{1080}, false, format, usage);

// Host-visible buffer operations
buffer.write(data, offset);
auto result = buffer.read_as_vector(offset, count);
```

## Staging Uploads

```cpp
StagingBufferManager staging(allocator);
staging.upload(deviceBuffer, data.data(), size, 0);
staging.flush(cmd, queue);
```

## Random Sampling (for RT)

```cpp
auto samples = create_hemisphere_samples_buffer(*allocator);
NoiseTexture noise(device, allocator, queue);
```
