# Memory Module

`vw` namespace · `Memory/` directory · Tier 4

GPU memory management via VMA (Vulkan Memory Allocator). Type-safe buffers with compile-time usage validation, staging uploads, and transfer operations.

## Allocator / AllocatorBuilder

```cpp
auto allocator = AllocatorBuilder(instance, device).build();
auto image = allocator->create_image_2D(Width{w}, Height{h}, false, fmt, usage);
```

## Buffer\<T, HostVisible, Flags\>

Type-safe GPU buffer with compile-time constraints:

```cpp
auto buf = create_buffer<float, true, UniformBufferUsage>(*allocator, 100);
buf.write(data, offset);                    // only when HostVisible == true
auto vec = buf.read_as_vector(offset, n);   // only when HostVisible == true
```

- `T` — element type, `HostVisible` — CPU-accessible, `Flags` — `VkBufferUsageFlags`
- `does_support(usage)` — `consteval` usage flag check
- `BufferBase` — non-templated base with `write_bytes()`, `read_bytes()`, `device_address()`, `size_bytes()`

## BufferUsage Constants

`VertexBufferUsage`, `IndexBufferUsage`, `UniformBufferUsage`, `StorageBufferUsage`, `StagingBufferUsage`

## AllocateBufferUtils

Factory functions:
- `create_buffer<T, HostVisible, Usage>(allocator, count)`
- `allocate_vertex_buffer<VertexType, HostVisible>(allocator, count)`

## BufferList

Append-only buffer that grows by allocating new segments. Used by MeshManager for vertex/index data.

## StagingBufferManager

Batch GPU uploads via staging buffers:
- `fill_buffer(data, dst, offset)` — queue a buffer upload
- `fill_command_buffer()` — record all queued uploads into a command buffer

## Transfer

Copy operations with automatic barrier management via embedded ResourceTracker:

```cpp
vw::Transfer transfer;
transfer.blit(cmd, srcImage, dstImage);
transfer.copyBufferToImage(cmd, buffer, image, offset);
transfer.saveToFile(cmd, allocator, queue, image, "out.png");
auto& tracker = transfer.resourceTracker();
```

`saveToFile` handles staging buffer creation, copy, format conversion (BGRA→RGBA), and file writing.

## UniformBufferAllocator

Sub-allocates uniform buffer memory with proper alignment.

## Anti-Patterns

- Never use raw `vkAllocateMemory` or `vkCreateBuffer` — use Allocator and `create_buffer<>()`
