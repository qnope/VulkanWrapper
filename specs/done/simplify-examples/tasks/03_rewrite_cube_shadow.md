# Task 3: Rewrite CubeShadow Example

## Summary

Rewrite `examples/CubeShadow/main.cpp` to use `ExampleRunner`. CubeShadow uses `RenderPipeline` with ZPass, DirectLightPass, SkyPass, and ToneMappingPass. It blits the tone-mapped output to the swapchain and screenshots at frame 3.

## Target: ~100 lines

## Implementation Steps

### 1. Create `CubeShadowExample` class

```cpp
class CubeShadowExample : public ExampleRunner {
    // Members (created in setup()):
    vw::Model::MeshManager mesh_manager;
    vw::rt::RayTracedScene ray_traced_scene;
    CameraConfig camera;
    vw::Buffer<UBOData, true, vw::UniformBufferUsage> uniform_buffer;
    vw::RenderPipeline pipeline;
    vw::SkyParameters sky_params;

    // Pass references (set during setup)
    vw::ZPass *zpass;
    vw::DirectLightPass *direct_light;
    vw::SkyPass *sky_pass;
    vw::ToneMappingPass *tone_mapping;

    void setup() override;
    void render(vw::CommandBuffer &cmd,
                vw::Transfer &transfer,
                uint32_t frame_index) override;
    std::shared_ptr<const vw::ImageView> result_image() override;
    void on_resize(vw::Width width, vw::Height height) override;
    // should_screenshot() → default (frame 3)
};
```

### 2. Constructor

Pass `App&` to `ExampleRunner`. Initialize `mesh_manager` and `ray_traced_scene` with device/allocator from app.

### 3. `setup()` implementation

Move from current main():
- Create cube+plane scene via `create_cube_plane_scene()`
- Create UBO with camera matrices
- Call `setup_ray_tracing()` to upload, instance, build BLAS/TLAS
- Track initial texture states via `transfer().resourceTracker().track()`
- Create RenderPipeline: add ZPass, DirectLightPass, SkyPass, ToneMappingPass (indirect_enabled=false)
- Validate pipeline
- Configure sky params: sun at 45deg, custom star_direction

### 4. `render()` implementation

Move from current frame loop body:
- Read UBO data, extract camera position
- Configure zpass: set_uniform_buffer, set_scene
- Configure directLight: set_uniform_buffer, set_sky_parameters, set_camera_position, set_frame_count(0)
- Configure skyPass: set_sky_parameters, set_inverse_view_proj
- Configure toneMapping: ACES, exposure=1, white_point=4, luminance_scale=10000, indirect_intensity=0
- Execute pipeline: `pipeline.execute(cmd, transfer.resourceTracker(), width, height, frame_index)`

Note: width/height come from `app().swapchain.width()` / `app().swapchain.height()`.

### 5. `result_image()` implementation

Find ToneMapped slot in `tone_mapping->result_images()` and return its view:
```cpp
for (const auto &[slot, cached] : tone_mapping->result_images()) {
    if (slot == vw::Slot::ToneMapped)
        return cached.view;
}
return nullptr;
```

### 6. `on_resize()` implementation

- `pipeline.reset_accumulation()`
- Update UBO with new aspect ratio: `uniform_buffer.write(UBOData::create(new_aspect, camera.view_matrix()), 0)`

### 7. `main()` becomes minimal

```cpp
int main() {
    App app;
    CubeShadowExample example(app);
    example.run();
}
```

## Files to Modify

### `examples/CubeShadow/main.cpp`
- Complete rewrite: replace 319-line main() with ~100-line class + minimal main()

### `examples/CubeShadow/CMakeLists.txt`
- Simplify link line: `ExamplesCommon` already provides `App` transitively
```cmake
add_executable(CubeShadow main.cpp)
target_link_libraries(CubeShadow PRIVATE ExamplesCommon)
target_precompile_headers(CubeShadow REUSE_FROM VulkanWrapperCoreLibrary)
```

## Dependencies

- Task 1 (ExampleRunner base class)
- `Common/SceneSetup.h` (unchanged, provides CameraConfig, UBOData, create_cube_plane_scene, create_ubo, setup_ray_tracing)

## Test Plan

1. Build: `cmake --build build --target CubeShadow`
2. Run: `cd build/examples/CubeShadow && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./CubeShadow`
3. Verify `screenshot.png` is produced
4. Compare screenshot with pre-refactor version (should be pixel-identical)
5. Verify it screenshots at frame 3
