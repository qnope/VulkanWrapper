# Task 4: Rewrite AmbientOcclusion Example

## Summary

Rewrite `examples/AmbientOcclusion/main.cpp` to use `ExampleRunner`. AmbientOcclusion uses `RenderPipeline` with ZPass, DirectLightPass, and AmbientOcclusionPass. It blits the AO output directly to the swapchain (no tone mapping) and screenshots after 256 AO samples.

## Target: ~100 lines

## Implementation Steps

### 1. Create `AOExample` class

```cpp
class AOExample : public ExampleRunner {
    // Members:
    vw::Model::MeshManager mesh_manager;
    vw::rt::RayTracedScene ray_traced_scene;
    CameraConfig camera;
    vw::Buffer<UBOData, true, vw::UniformBufferUsage> uniform_buffer;
    vw::RenderPipeline pipeline;

    // Pass references:
    vw::ZPass *zpass;
    vw::DirectLightPass *direct_light;
    vw::AmbientOcclusionPass *ao_pass;

    void setup() override;
    void render(vw::CommandBuffer &cmd,
                vw::Transfer &transfer,
                uint32_t frame_index) override;
    std::shared_ptr<const vw::ImageView> result_image() override;
    void on_resize(vw::Width width, vw::Height height) override;
    bool should_screenshot() const override;
};
```

### 2. Constructor

Initialize `mesh_manager` and `ray_traced_scene` from `app`.

### 3. `setup()` implementation

- Create cube+plane scene via `create_cube_plane_scene()`
- Create UBO with camera matrices
- Call `setup_ray_tracing()`
- Track initial texture states
- Create RenderPipeline: ZPass -> DirectLightPass -> AmbientOcclusionPass
- Validate pipeline

### 4. `render()` implementation

- Create sky params: `SkyParameters::create_earth_sun(0.8f)`
- Read UBO data, extract camera position
- Configure zpass: set_uniform_buffer, set_scene
- Configure directLight: set_uniform_buffer, set_sky_parameters, set_camera_position, set_frame_count(0)
- Configure aoPass: set_ao_radius(2.0f)
- Execute pipeline

### 5. `result_image()` implementation

Find AmbientOcclusion slot in `ao_pass->result_images()`:
```cpp
for (const auto &[slot, cached] : ao_pass->result_images()) {
    if (slot == vw::Slot::AmbientOcclusion)
        return cached.view;
}
return nullptr;
```

### 6. `on_resize()` implementation

- `pipeline.reset_accumulation()`
- Update UBO with new aspect ratio

### 7. `should_screenshot()` implementation

```cpp
return ao_pass->get_frame_count() == 256;
```

### 8. `main()` becomes minimal

```cpp
int main() {
    App app;
    AOExample example(app);
    example.run();
}
```

## Files to Modify

### `examples/AmbientOcclusion/main.cpp`
- Complete rewrite: replace 243-line main() with ~100-line class + minimal main()

### `examples/AmbientOcclusion/CMakeLists.txt`
- Simplify link line:
```cmake
add_executable(AmbientOcclusion main.cpp)
target_link_libraries(AmbientOcclusion PRIVATE ExamplesCommon)
target_precompile_headers(AmbientOcclusion REUSE_FROM VulkanWrapperCoreLibrary)
```

## Dependencies

- Task 1 (ExampleRunner base class)
- `Common/SceneSetup.h`

## Test Plan

1. Build: `cmake --build build --target AmbientOcclusion`
2. Run: `cd build/examples/AmbientOcclusion && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./AmbientOcclusion`
3. Verify `screenshot.png` is produced
4. Compare screenshot with pre-refactor version (should be pixel-identical)
5. Verify it screenshots after 256 AO samples (not at frame 3)
6. Verify "Iteration: N" is printed each frame
