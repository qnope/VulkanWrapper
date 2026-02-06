# Dev Skill

Quick reference for VulkanWrapper development. See CLAUDE.md for build commands and core patterns.

## Core Rules

- **C++23**: Concepts over SFINAE, ranges over raw loops, `requires` clauses
- **Vulkan 1.3**: Dynamic rendering (`cmd.beginRendering()`), Synchronization2 (`vk::PipelineStageFlagBits2`), no `VkRenderPass`
- **RAII**: All resources auto-managed via builders -- never call destroy/free manually
- **Type safety**: `Buffer<T, bool HostVisible, VkBufferUsageFlags Flags>`, strong types (`Width`, `Height`, `MipLevel`)
- **Barriers**: Always via `ResourceTracker` (`vw::Barrier` namespace) -- never raw `vkCmdPipelineBarrier`

## Anti-Patterns (DO NOT)

See CLAUDE.md "Anti-Patterns" section for the full list. Key ones:
- No v1 pipeline stage/access flags -- use `vk::PipelineStageFlagBits2` / `vk::AccessFlags2`
- No `cmd.beginRenderPass()` -- use `cmd.beginRendering()`
- No raw `vkAllocateMemory` / `vkCreateBuffer` -- use `Allocator` + `create_buffer<>()`
- No `Vk` prefix C types -- use `vk::` C++ bindings
- No SFINAE -- use concepts and `requires`
- No raw loops when ranges/algorithms work

## Quick Reference

| Topic | File | Key Points |
|-------|------|------------|
| Memory | [memory-allocation.md](memory-allocation.md) | Allocator, Buffer types, staging, image creation |
| Barriers | [barriers.md](barriers.md) | ResourceTracker, Transfer, common transitions |
| Patterns | [patterns.md](patterns.md) | ScreenSpacePass, MeshRenderer, RAII handles, descriptors |
| C++23 | [modern-cpp.md](modern-cpp.md) | Concepts, ranges, consteval, to_handle adaptor |
| Shaders | [shaders.md](shaders.md) | ShaderCompiler, GLSL includes, push constants |
| Ray Tracing | [ray-tracing.md](ray-tracing.md) | BLAS/TLAS, RayTracedScene, SBT, GeometryReference |
| Vulkan | [vulkan-features.md](vulkan-features.md) | Device setup, feature negotiation, dynamic rendering |

## Common Workflow

1. Read existing code in `VulkanWrapper/include/VulkanWrapper/<Module>/`
2. Write tests first (see `/test` skill)
3. Implement following existing patterns
4. Enable validation layers: `InstanceBuilder().setDebug()`
5. Format: `git diff --name-only -- '*.h' '*.cpp' | xargs -r clang-format -i`
6. Test: `cd build-Clang20Debug && ctest --output-on-failure`
