# Dev Skill

Quick reference for VulkanWrapper development.

## Core Rules

See CLAUDE.md for code style, build commands, and core patterns. Key rules:
- **C++23**: Concepts over SFINAE, ranges over loops
- **Vulkan 1.3**: Dynamic rendering, Synchronization2, no VkRenderPass
- **RAII**: All resources auto-managed via builders
- **Type safety**: `Buffer<T, bool HostVisible, VkBufferUsageFlags Flags>`, strong types (`Width`, `Height`, `MipLevel`)

## Quick Reference

| Topic | File | Key Points |
|-------|------|------------|
| Memory | [memory-allocation.md](memory-allocation.md) | Allocator, Buffer types, staging |
| Barriers | [barriers.md](barriers.md) | ResourceTracker, Transfer |
| Patterns | [patterns.md](patterns.md) | ScreenSpacePass, RAII handles |
| C++23 | [modern-cpp.md](modern-cpp.md) | Concepts, ranges, consteval |
| Shaders | [shaders.md](shaders.md) | ShaderCompiler, push constants |
| Ray Tracing | [ray-tracing.md](ray-tracing.md) | BLAS/TLAS, RayTracedScene |
| Vulkan | [vulkan-features.md](vulkan-features.md) | Device setup, features |
