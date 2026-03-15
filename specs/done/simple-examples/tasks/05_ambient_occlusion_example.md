# Task 5: AmbientOcclusion Example

## Summary

Implement `examples/AmbientOcclusion/`, the same cube+plane scene as CubeShadow but displaying the raw ambient occlusion buffer in grayscale. Uses `ZPass -> DirectLightPass -> AmbientOcclusionPass`, then blits the AO image directly to the swapchain.

## Dependencies

- **Task 1** (Primitive Geometry Library): For geometry generation.
- **Task 2** (Common Scene Setup): For `create_cube_plane_scene()`, `setup_ray_tracing()`.

## Implementation Steps

### 1. Create `examples/AmbientOcclusion/main.cpp`

**Main flow (same as CubeShadow for setup, differs in pipeline and output):**

1. **Setup**: `App`, `MeshManager`, `RayTracedScene`, `create_cube_plane_scene()`, upload, build AS.
2. **Create render pipeline** (shorter than CubeShadow — no Sky or ToneMapping):
   ```cpp
   const std::filesystem::path shader_dir = "../../../VulkanWrapper/Shaders";
   vw::RenderPipeline pipeline;

   auto &zpass = pipeline.add(
       std::make_unique<vw::ZPass>(app.device, app.allocator, shader_dir));
   auto &directLight = pipeline.add(
       std::make_unique<vw::DirectLightPass>(
           app.device, app.allocator, shader_dir, ray_traced_scene,
           mesh_manager.material_manager()));
   auto &aoPass = pipeline.add(
       std::make_unique<vw::AmbientOcclusionPass>(
           app.device, app.allocator, shader_dir,
           ray_traced_scene.tlas_handle()));

   pipeline.validate();
   ```
3. **Render loop**:
   - Per-frame pass configuration:
     - `zpass.set_uniform_buffer(...)`, `zpass.set_scene(...)`
     - `directLight.set_uniform_buffer(...)`, `.set_sky_parameters(...)`, `.set_camera_position(...)`
     - `aoPass.set_ao_radius(...)` — tune for the scene scale (primitives are unit-sized, scene ~20 units wide, so radius ≈ 2.0-5.0)
   - `pipeline.execute(cmd, tracker, width, height, frame_index)`
   - **Blit AO output directly to swapchain**:
     ```cpp
     auto ao_results = aoPass.result_images();
     std::shared_ptr<const vw::ImageView> ao_view;
     for (const auto &[slot, cached] : ao_results) {
         if (slot == vw::Slot::AmbientOcclusion) {
             ao_view = cached.view;
             break;
         }
     }
     transfer.blit(cmd, ao_view->image(), swapchain_image_view->image());
     ```
   - Transition swapchain to present, flush tracker
4. **Screenshot timing**: AO is progressive (1 sample/pixel/frame). Wait for 32 frames for clean AO, same as Advanced example:
   ```cpp
   if (aoPass.get_frame_count() == 32) {
       // Save screenshot and break
   }
   ```

**Key difference from CubeShadow**: Instead of routing through ToneMappingPass and blitting the tone-mapped output, we directly blit the AO slot image to the swapchain. The AO image is single-channel (R32Sfloat or similar) — the blit will replicate R to RGB, producing grayscale output.

### 2. Create `examples/AmbientOcclusion/CMakeLists.txt`

```cmake
add_executable(AmbientOcclusion main.cpp)
target_link_libraries(AmbientOcclusion PRIVATE App ExamplesCommon)
target_precompile_headers(AmbientOcclusion REUSE_FROM VulkanWrapperCoreLibrary)
```

### 3. Register in `examples/CMakeLists.txt`

Add `add_subdirectory(AmbientOcclusion)`.

## Design Considerations

- **Direct blit for AO visualization**: The simplest approach — `Transfer::blit()` handles format conversion. The AO image format might be `R8Unorm` or `R32Sfloat`; either way, blitting to the swapchain (typically `B8G8R8A8Srgb`) will produce a grayscale image since R is replicated across channels by the blit operation.
  - **Caveat**: If the blit doesn't replicate R to all channels automatically (depends on format), we may need to check the AO image format. If it's a multi-channel format like `R16G16B16A16Sfloat` where AO is in the R channel, the blit will work correctly. If there are issues, fall back to a simple fullscreen pass.
- **AO radius**: Must be tuned relative to scene scale. The cube is ~1 unit, plane is ~20 units. An AO radius of 2.0-5.0 should show visible occlusion at the cube-plane contact area.
- **Progressive accumulation**: AO accumulates over frames. The pipeline's `reset_accumulation()` is called on swapchain resize. Screenshot at frame 32 gives clean results.

## Test Plan

- **Build validation**: `cmake --build build --target AmbientOcclusion` compiles.
- **Visual validation**: `screenshot.png` shows grayscale AO — dark areas where the cube meets/shadows the plane, white elsewhere.
