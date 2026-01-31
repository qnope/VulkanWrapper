---
sidebar_position: 1
---

# Command Pool

The `CommandPool` class manages command buffer allocation.

## Overview

```cpp
#include <VulkanWrapper/Command/CommandPool.h>

auto pool = CommandPoolBuilder()
    .setDevice(device)
    .setQueueFamilyIndex(device->graphics_queue().family_index())
    .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .build();
```

## CommandPoolBuilder

### Methods

| Method | Description |
|--------|-------------|
| `setDevice(device)` | Set the logical device |
| `setQueueFamilyIndex(index)` | Set queue family for commands |
| `setFlags(flags)` | Set pool creation flags |

### Pool Flags

| Flag | Description |
|------|-------------|
| `eResetCommandBuffer` | Allow individual buffer reset |
| `eTransient` | Buffers are short-lived |
| `eProtected` | Protected command buffers |

### Example

```cpp
// For reusable command buffers
auto graphicsPool = CommandPoolBuilder()
    .setDevice(device)
    .setQueueFamilyIndex(device->graphics_queue().family_index())
    .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .build();

// For one-time transfers
auto transferPool = CommandPoolBuilder()
    .setDevice(device)
    .setQueueFamilyIndex(device->transfer_queue().family_index())
    .setFlags(vk::CommandPoolCreateFlagBits::eTransient)
    .build();
```

## CommandPool Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::CommandPool` | Get raw handle |
| `allocate()` | `CommandBuffer` | Allocate one primary buffer |
| `allocate(count)` | `vector<CommandBuffer>` | Allocate multiple buffers |
| `allocate_secondary()` | `CommandBuffer` | Allocate secondary buffer |
| `reset()` | `void` | Reset all buffers |

### Allocating Buffers

```cpp
// Single buffer
auto cmd = pool->allocate();

// Multiple buffers
auto cmds = pool->allocate(3);  // For triple buffering

// Secondary buffer
auto secondary = pool->allocate_secondary();
```

### Resetting Pool

```cpp
// Reset all command buffers at once
pool->reset();

// This is more efficient than resetting individually
// when you want to reset all buffers
```

## Queue Family Selection

Command buffers must be submitted to queues of the same family:

```cpp
// Graphics commands
auto graphicsPool = CommandPoolBuilder()
    .setQueueFamilyIndex(device->graphics_queue().family_index())
    .build();

// Compute commands (may be different family)
auto computePool = CommandPoolBuilder()
    .setQueueFamilyIndex(device->compute_queue().family_index())
    .build();

// Transfer commands
auto transferPool = CommandPoolBuilder()
    .setQueueFamilyIndex(device->transfer_queue().family_index())
    .build();
```

## Per-Frame Pools

Common pattern for frame buffering:

```cpp
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

std::array<std::shared_ptr<CommandPool>, FRAMES_IN_FLIGHT> pools;
std::array<CommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;

for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
    pools[i] = CommandPoolBuilder()
        .setDevice(device)
        .setQueueFamilyIndex(graphicsFamily)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .build();

    commandBuffers[i] = pools[i]->allocate();
}
```

## Best Practices

1. **One pool per thread** - pools are not thread-safe
2. **Use appropriate flags** for usage pattern
3. **Reset pool** instead of individual buffers when possible
4. **Match queue family** to submission queue
5. **Use transient** for one-time commands
