# Dev Skill

Quick reference for VulkanWrapper development.

## Core Rules

- **C++23**: Concepts over SFINAE, ranges over loops
- **Vulkan 1.3**: Dynamic rendering, Synchronization2, no VkRenderPass
- **RAII**: All resources auto-managed
- **Type safety**: `Buffer<T, HostVisible, Usage>`, strong types

## Quick Reference

| Topic | File | Key Points |
|-------|------|------------|
| Memory | [memory-allocation.md](memory-allocation.md) | Allocator, Buffer types |
| Barriers | [barriers.md](barriers.md) | ResourceTracker |
| Patterns | [patterns.md](patterns.md) | ScreenSpacePass, builders |
| C++23 | [modern-cpp.md](modern-cpp.md) | Concepts, ranges |
| Shaders | [shaders.md](shaders.md) | ShaderCompiler, push constants |
| Ray Tracing | [ray-tracing.md](ray-tracing.md) | BLAS/TLAS, RayTracedScene |
| Vulkan | [vulkan-features.md](vulkan-features.md) | Device setup, features |

## Anti-Patterns

| DON'T | DO |
|-------|-----|
| `std::enable_if_t<...>` | `requires(Concept)` |
| `cmd.pipelineBarrier(...)` | `tracker.request(...); tracker.flush(cmd);` |
| `vkCmdBeginRenderPass(...)` | `cmd.beginRendering(...)` |
| `vkCreateBuffer + vkAllocateMemory` | `allocator->allocate_buffer(...)` |
