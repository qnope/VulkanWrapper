# Tests

`VulkanWrapper/tests/` directory

GTest-based test suite running on a shared GPU singleton. Tests run on llvmpipe (software Vulkan driver) for CI compatibility and full ray tracing support.

## GPU Singleton

```cpp
auto& gpu = vw::tests::create_gpu();  // shared across all tests
// gpu.instance, gpu.device, gpu.allocator, gpu.queue()
```

For ray tracing tests (defined per-test-file, not shared):
```cpp
auto* gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```

## Test Organization

| Directory | Tests | Target |
|-----------|-------|--------|
| Image/ | ImageTests, ImageViewTests, SamplerTests | ImageTests |
| Material/ | BindlessMaterialManager, BindlessTextureManager, ColoredMaterialHandler, EmissiveTexturedMaterialHandler, MaterialTypeTag, TexturedMaterialHandler | MaterialTests |
| Memory/ | Allocator, Buffer, IntervalSet, Interval, ResourceTracker, StagingBufferManager, Transfer, UniformBufferAllocator | MemoryTests |
| Model/ | MeshManagerTests | ModelTests |
| Random/ | RandomSamplingTests | RandomTests |
| RayTracing/ | GeometryAccessTests, RayTracedSceneTests | RayTracingTests |
| RenderPass/ | AmbientOcclusionPass, DirectLightPass, IndirectLightPass, IndirectLightPassSunBounce, RenderPipeline, ScreenSpacePass, SkyPass, Subpass, ToneMappingPass, ZPass | RenderPassTests |
| Shader/ | ShaderCompilerTests | ShaderTests |
| Vulkan/ | InstanceTests | VulkanTests |

## Running Tests

```bash
# All tests
cd build && DYLD_LIBRARY_PATH=vcpkg_installed/arm64-osx/lib ctest --output-on-failure -j8

# Specific suite
cd build/VulkanWrapper/tests && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./RenderPassTests

# Specific test
cd build/VulkanWrapper/tests && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./RenderPassTests --gtest_filter='*IndirectLight*'
```
