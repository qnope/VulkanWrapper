---
sidebar_position: 1
title: "Command Pool"
---

# Command Pool

## Overview

A **command pool** is a Vulkan object that manages memory for command buffers.
Every command buffer you record must come from a command pool.
VulkanWrapper provides `CommandPool` and `CommandPoolBuilder` as thin, RAII-safe
wrappers around the native Vulkan handles.

### Why command pools exist

The GPU cannot execute function calls the way a CPU does.
Instead, you build a *command buffer* -- a pre-recorded list of GPU instructions
-- and submit it to a *queue*.
The command pool is the memory manager that backs those command buffers.

Key facts:

- A command pool is tied to a single **queue family** (graphics, compute,
  transfer, ...).  Command buffers allocated from it can only be submitted to
  queues of that family.
- Command pools are **not thread-safe**.  If you record commands on multiple
  threads, create one pool per thread.
- Resetting a pool recycles the memory of *every* command buffer allocated from
  it in one shot -- much cheaper than resetting each buffer individually.

### Where it lives in the library

| Item | Header |
|------|--------|
| `CommandPool` | `VulkanWrapper/Command/CommandPool.h` |
| `CommandPoolBuilder` | `VulkanWrapper/Command/CommandPool.h` |

Both classes live in the `vw` namespace.

---

## API Reference

### CommandPoolBuilder

The builder follows the same fluent pattern used throughout VulkanWrapper.

| Method | Returns | Description |
|--------|---------|-------------|
| `CommandPoolBuilder(std::shared_ptr<const Device> device)` | -- | Constructs the builder. The device's graphics queue family is used by default. |
| `with_reset_command_buffer()` | `CommandPoolBuilder&` | Sets the `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` flag, allowing individual command buffers to be reset and re-recorded without resetting the whole pool. |
| `build()` | `CommandPool` | Creates the Vulkan command pool. |

### CommandPool

`CommandPool` inherits `ObjectWithUniqueHandle<vk::UniqueCommandPool>`, which
means it owns the underlying Vulkan handle and destroys it automatically when
the object goes out of scope.

| Method | Returns | Description |
|--------|---------|-------------|
| `allocate(std::size_t number)` | `std::vector<vk::CommandBuffer>` | Allocates `number` **primary** command buffers from the pool. |
| `handle()` | `vk::CommandPool` | The raw Vulkan handle (inherited). |

The returned `vk::CommandBuffer` handles are lightweight -- they do not own
their memory.  The pool owns them, and they become invalid if the pool is
destroyed.

---

## Usage Examples

### Creating a pool and allocating buffers

```cpp
// Obtain a device (typically from DeviceFinder)
auto device = /* ... */;

// Build a command pool that allows per-buffer reset
auto pool = vw::CommandPoolBuilder(device)
                .with_reset_command_buffer()
                .build();

// Allocate two primary command buffers
auto cmds = pool.allocate(2);
vk::CommandBuffer cmd0 = cmds[0];
vk::CommandBuffer cmd1 = cmds[1];
```

### Recording and submitting a single command buffer

```cpp
auto pool = vw::CommandPoolBuilder(device).build();
auto cmd  = pool.allocate(1)[0];

// Begin recording (one-time submit for transient work)
std::ignore = cmd.begin(
    vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

// ... record commands ...

std::ignore = cmd.end();

// Submit to the queue
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();
```

### Using CommandBufferRecorder for RAII recording

Instead of manually calling `begin()` / `end()`, you can use
`CommandBufferRecorder` (see [Command Buffer](./command-buffer.md)).

---

## Integration Patterns

### One pool per frame-in-flight

A common pattern in real-time rendering is to have N frames in flight and one
command pool per frame.  At the start of each frame you can re-allocate
from the pool or reset individual buffers:

```cpp
// During initialization
std::vector<vw::CommandPool> pools;
for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
    pools.push_back(
        vw::CommandPoolBuilder(device)
            .with_reset_command_buffer()
            .build());
}

// Each frame
auto& pool = pools[current_frame];
auto cmd = pool.allocate(1)[0];
```

### Thread-local pools for multithreaded recording

```cpp
// Each worker thread creates its own pool
thread_local auto pool =
    vw::CommandPoolBuilder(device)
        .with_reset_command_buffer()
        .build();
```

---

## Common Pitfalls

1. **Submitting to the wrong queue family.**
   A command pool is bound to one queue family at creation time.
   If you allocate a buffer from a *graphics* pool and try to submit it to a
   *compute-only* queue, you will get a validation error or undefined behavior.

2. **Forgetting `with_reset_command_buffer()`.**
   Without this flag, calling `cmd.reset()` on an individual command buffer is
   invalid.  If you plan to re-record the same buffer each frame, enable this
   flag.  If you always allocate fresh buffers, you may not need it.

3. **Using the pool from multiple threads.**
   Vulkan command pools are *externally synchronized*.
   Never allocate or record command buffers from the same pool concurrently.
   Create one pool per thread, or protect access with a mutex.

4. **Destroying the pool while command buffers are in flight.**
   Make sure the GPU has finished executing all command buffers before the pool
   is destroyed (e.g., wait on the submission fence first).
