# Command Module

`vw` namespace · `Command/` directory · Tier 3

Command buffer management for GPU command recording.

## CommandPool / CommandPoolBuilder

```cpp
auto pool = CommandPoolBuilder(device)
    .with_reset_command_buffer()  // enable per-buffer reset (for per-frame re-recording)
    .build();
auto cmds = pool.allocate(3);  // allocate N command buffers
```

## CommandBuffer

Thin wrapper around `vk::CommandBuffer`:
- `begin()` / `end()` — manual recording control
- `reset()` — reset for per-frame re-recording

## CommandBufferRecorder

RAII scoped recorder — calls `begin()` on construct, `end()` on destruct:

```cpp
{
    CommandBufferRecorder recorder(cmds[0]);
    // record commands...
} // auto-ends the command buffer
```

Depends on: Vulkan module (Device, Queue).
