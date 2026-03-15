# Task 4: CubeShadow Example

## Summary

Implement `examples/CubeShadow/`, a deferred rendering example showing a cube floating above a ground plane, lit by the sun with a visible shadow. Uses the full `RenderPipeline` with `ZPass -> DirectLightPass -> SkyPass -> ToneMappingPass`.

## Dependencies

- **Task 1** (Primitive Geometry Library): For `create_square()`, `create_cube()`.
- **Task 2** (Common Scene Setup): For `create_cube_plane_scene()`, `create_ubo()`, `setup_ray_tracing()`.

## Implementation Steps

### 1. Create `examples/CubeShadow/main.cpp`

Follow the pattern of `examples/Advanced/main.cpp` but simplified:

**Main flow:**

1. **Setup**: Create `App` instance.
2. **Scene**:
   ```cpp
   vw::Model::MeshManager mesh_manager(app.device, app.allocator);
   vw::rt::RayTracedScene ray_traced_scene(app.device, app.allocator);
   auto scene_config = create_cube_plane_scene(mesh_manager);
   ```
3. **UBO**: `create_ubo(*app.allocator, aspect, scene_config.camera)`.
4. **Upload & ray tracing**: `setup_ray_tracing(mesh_manager, ray_traced_scene, *app.device, scene_config.instances)`.
5. **Track initial texture states**:
   ```cpp
   vw::Transfer transfer;
   for (const auto &resource : mesh_manager.material_manager().get_resources())
       transfer.resourceTracker().track(resource);
   ```
6. **Create render pipeline**:
   ```cpp
   const std::filesystem::path shader_dir = "../../../VulkanWrapper/Shaders";
   vw::RenderPipeline pipeline;

   auto &zpass = pipeline.add(
       std::make_unique<vw::ZPass>(app.device, app.allocator, shader_dir));
   auto &directLight = pipeline.add(
       std::make_unique<vw::DirectLightPass>(
           app.device, app.allocator, shader_dir, ray_traced_scene,
           mesh_manager.material_manager()));
   auto &skyPass = pipeline.add(
       std::make_unique<vw::SkyPass>(app.device, app.allocator, shader_dir));
   auto &toneMapping = pipeline.add(
       std::make_unique<vw::ToneMappingPass>(
           app.device, app.allocator, shader_dir));

   auto result = pipeline.validate();
   // Handle validation errors...
   ```
7. **Command pool, semaphores**: Same pattern as Advanced.
8. **Render loop**:
   - Per-frame configuration:
     - `zpass.set_uniform_buffer(uniform_buffer)` and `zpass.set_scene(ray_traced_scene)`
     - `directLight.set_uniform_buffer(uniform_buffer)`, `.set_sky_parameters(sky_params)`, `.set_camera_position(camera_pos)`
     - `skyPass.set_sky_parameters(sky_params)`, `.set_inverse_view_proj(ubo_data.inverseViewProj)`
     - `toneMapping.set_operator(vw::ToneMappingOperator::ACES)`, `.set_exposure(1.0f)`, etc.
   - `pipeline.execute(cmd, tracker, width, height, frame_index)`
   - Blit tone-mapped output to swapchain (same pattern as Advanced)
   - Transition swapchain to present
9. **Screenshot**: After a few frames (e.g., frame 3 — no progressive accumulation needed since no AO/indirect light), save and exit.

**Sun configuration:**
- Use `SkyParameters::create_earth_sun(angle)` with an angle that creates a visible shadow. The sun should be somewhat low (e.g., angle ≈ 0.5-1.0 radians) so the shadow is elongated and visible on the ground plane.
- The `directLight.set_frame_count(0)` can be set to 0 since there's no indirect light pass.

### 2. Create `examples/CubeShadow/CMakeLists.txt`

```cmake
add_executable(CubeShadow main.cpp)
target_link_libraries(CubeShadow PRIVATE App ExamplesCommon)
target_precompile_headers(CubeShadow REUSE_FROM VulkanWrapperCoreLibrary)
```

### 3. Register in `examples/CMakeLists.txt`

Add `add_subdirectory(CubeShadow)`.

## Design Considerations

- **No AO or indirect light**: This example deliberately uses only direct sun lighting to keep the pipeline simple and demonstrate shadow casting.
- **Sun angle**: Must be tuned so the shadow of the cube is clearly visible on the ground plane. Too high = shadow directly under cube (hard to see). Too low = very long shadow that may go off-screen.
- **Screenshot timing**: Since there are no progressive passes (AO, indirect light), the scene is fully converged on the first frame. Take screenshot after 2-3 frames to ensure all passes have executed correctly.

## Test Plan

- **Build validation**: `cmake --build build --target CubeShadow` compiles.
- **Visual validation**: `screenshot.png` shows a cube above a plane with a clear sun shadow on the ground, sky in background.
