# Task 7: Update Advanced Example + Cleanup

## Summary

Update `examples/Advanced/` to use the library passes and `RenderPipeline` instead of the local pass implementations and `DeferredRenderingManager`. Remove all duplicated code. Verify that the rendering output (`screenshot.png`) is visually identical to the current output.

---

## Implementation Steps

### 7.1 Remove example pass files

**Delete the following files from `examples/Advanced/`:**
- `ZPass.h`
- `DirectLightPass.h`
- `AmbientOcclusionPass.h`
- `DeferredRenderingManager.h`

These are now in the library under `VulkanWrapper/include/VulkanWrapper/RenderPass/`.

### 7.2 Update `RenderPassInformation.h`

**File:** `examples/Advanced/RenderPassInformation.h`

**Remove:**
- `struct GBuffer` — replaced by slot-based outputs
- `struct GBufferInformation` — no longer needed
- `struct TonemapInformation` — no longer needed

**Keep:**
- `struct UBOData` — application-specific (proj, view, inverseViewProj matrices)
- `struct PushConstantData` — application-specific (model matrix)

These structs are used by `main.cpp` for camera setup and are not pass-specific.

### 7.3 Update `main.cpp`

**File:** `examples/Advanced/main.cpp`

Replace the `DeferredRenderingManager` usage with `RenderPipeline`:

**Before (current):**
```cpp
auto renderingManager = std::make_unique<DeferredRenderingManager>(...);
// Per-frame:
auto ldr_view = renderingManager->execute(cmd, tracker, width, height,
                                          frame_index, uniform_buffer,
                                          sky_params, ao_radius);
transfer.blit(cmd, ldr_view->image(), swapchain_image);
```

**After (new):**
```cpp
// Setup — create pipeline and add passes
vw::RenderPipeline pipeline;
auto& zpass = pipeline.add(std::make_unique<vw::ZPass>(device, allocator, shader_dir));
auto& directLight = pipeline.add(std::make_unique<vw::DirectLightPass>(
    device, allocator, shader_dir, ray_traced_scene, material_manager));
auto& aoPass = pipeline.add(std::make_unique<vw::AmbientOcclusionPass>(
    device, allocator, shader_dir, ray_traced_scene.tlas_handle()));
auto& skyPass = pipeline.add(std::make_unique<vw::SkyPass>(
    device, allocator, shader_dir));
auto& indirectLight = pipeline.add(std::make_unique<vw::IndirectLightPass>(
    device, allocator, shader_dir, ray_traced_scene.tlas_handle(),
    ray_traced_scene.geometry_buffer_address(), material_manager));
auto& toneMapping = pipeline.add(std::make_unique<vw::ToneMappingPass>(
    device, allocator, shader_dir));

auto result = pipeline.validate();
assert(result.valid);

// Per-frame:
auto sky_params = vw::SkyParameters::create_earth_sun(0.f);
auto ubo = UBOData::create(aspect_ratio, view_matrix);

zpass.set_uniform_buffer(uniform_buffer);
zpass.set_scene(ray_traced_scene);
directLight.set_uniform_buffer(uniform_buffer);
directLight.set_sky_parameters(sky_params);
directLight.set_camera_position(camera_pos);
directLight.set_frame_count(indirectLight.get_frame_count());
skyPass.set_sky_parameters(sky_params);
skyPass.set_inverse_view_proj(ubo.inverseViewProj);
indirectLight.set_sky_parameters(sky_params);
toneMapping.set_exposure(1.0f);
toneMapping.set_luminance_scale(10000.0f);
toneMapping.set_indirect_intensity(1.0f);

pipeline.execute(cmd, tracker, width, height, frame_index);

// Blit tone-mapped output to swapchain
auto final_images = toneMapping.result_images();
auto& tone_mapped = /* find Slot::ToneMapped in final_images */;
transfer.blit(cmd, tone_mapped.image, swapchain_image);
```

**Key changes:**
1. Replace `DeferredRenderingManager` construction with `RenderPipeline` + individual pass `add()` calls
2. Replace single `renderingManager->execute()` call with:
   - Per-pass setter calls for non-slot data
   - `pipeline.execute()` call
3. Replace `renderingManager->reset()` with `pipeline.reset_accumulation()`
4. Replace `renderingManager->ao_pass().get_frame_count()` with `aoPass.get_frame_count()`
5. Blit from `toneMapping.result_images()` instead of the returned `ldr_view`

### 7.4 Update `examples/Advanced/CMakeLists.txt`

Remove deleted files from the source list:

```cmake
add_executable(Main main.cpp
    RenderPassInformation.h
    SceneSetup.h
    # ZPass.h              ← REMOVED
    # DirectLightPass.h    ← REMOVED
    # AmbientOcclusionPass.h ← REMOVED
    # DeferredRenderingManager.h ← REMOVED
)
```

### 7.5 Update module files

**File:** `examples/Advanced/*.cppm` partitions

Remove partitions that referenced the deleted files:
- Remove `:z_pass` partition (was for the example's ZPass)
- Remove `:direct_light_pass` partition
- Remove `:ambient_occlusion_pass` partition (if it existed)
- Remove `:deferred_rendering_manager` partition

Update the Advanced aggregate `.cppm` to no longer re-export removed partitions.

`main.cpp` now only needs `import app; import vw;` (or `import advanced;` if still needed for SceneSetup).

### 7.6 Remove example shaders (already moved)

Verify that the following files have been moved to the library (Tasks 3-5) and delete the originals:
- `examples/Advanced/Shaders/GBuffer/zpass.vert`
- `examples/Advanced/Shaders/GBuffer/gbuffer.vert`
- `examples/Advanced/Shaders/GBuffer/gbuffer_base.glsl`
- `examples/Advanced/Shaders/post-process/ambient_occlusion.frag`

Keep `examples/Advanced/Shaders/quad.vert` only if it differs from `VulkanWrapper/Shaders/fullscreen.vert` and is still needed.

After cleanup, `examples/Advanced/Shaders/` should either be empty (and removed) or contain only example-specific shaders that aren't part of the library.

### 7.7 Integration test

Run the full pipeline:
```bash
cd build/examples/Advanced && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./Main
```

Compare `screenshot.png` with the reference output (current output before migration). The images should be visually identical (pixel-perfect or within floating-point tolerance).

---

## CMake Registration

No new files. Only deletions and modifications to existing CMakeLists.

---

## Dependencies

- **Requires:** All previous tasks (1-6)
- **Blocked by:** Tasks 1-6 (all passes and RenderPipeline must be in the library)
- **Blocks:** Nothing (final task)

---

## Test Plan

This task is primarily an integration test — no new GTest files.

| Validation Step | Description |
|-----------------|-------------|
| `cmake --build build` | Project compiles without errors |
| `ctest --stop-on-failure` | All existing + new tests pass |
| `./Main` produces screenshot | Running the Advanced example generates `screenshot.png` |
| Visual comparison | `screenshot.png` matches the pre-migration reference image |
| No duplicate code | No pass implementations remain in `examples/Advanced/` (only `main.cpp`, `RenderPassInformation.h`, `SceneSetup.h`) |
| Example stays minimal | `examples/Advanced/` contains only application-specific code (scene setup, camera, UBO, render loop) |

---

## Design Considerations

- **Shader directory path**: The library passes now load shaders from `VulkanWrapper/Shaders/`. The Advanced example must pass the correct `shader_dir` path to each pass constructor. This could be:
  1. A relative path from the executable (`../../VulkanWrapper/Shaders/`)
  2. A CMake-configured path (`${CMAKE_SOURCE_DIR}/VulkanWrapper/Shaders/`)
  3. Embedded in the binary

  **Recommended**: Use a CMake `configure_file` or `target_compile_definitions` to embed the shader directory path. The existing passes already receive a `shader_dir` parameter — keep this pattern.

- **SceneSetup.h**: This file contains scene loading configuration (mesh paths, transforms). It stays in the example as application-specific code.

- **RenderPassInformation.h cleanup**: After removing `GBuffer`, `GBufferInformation`, and `TonemapInformation`, the file contains only `UBOData` and `PushConstantData`. Consider renaming to `AppData.h` or keeping the name for git history clarity.

- **Progressive state coordination**: The example currently uses `renderingManager.ao_pass().get_frame_count()` to decide when to capture the screenshot (at 32 samples). With `RenderPipeline`, use `aoPass.get_frame_count()` directly (the reference is held from `pipeline.add()`).

- **Reset on resize**: The example calls `renderingManager.reset()` on swapchain recreation (resize). Replace with `pipeline.reset_accumulation()`.
