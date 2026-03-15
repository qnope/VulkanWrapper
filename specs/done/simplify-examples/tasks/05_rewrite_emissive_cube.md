# Task 5: Rewrite EmissiveCube Example

## Summary

Rewrite `examples/EmissiveCube/main.cpp` to use `ExampleRunner`. EmissiveCube uses the full `RenderPipeline` with all six passes (ZPass, DirectLightPass, AmbientOcclusionPass, SkyPass, IndirectLightPass, ToneMappingPass). It demonstrates emissive materials as light sources and screenshots after 256 AO samples.

## Target: ~120 lines

This is the longest example because it has custom scene setup (emissive material, two different material types) and the most passes to configure.

## Implementation Steps

### 1. Create `EmissiveCubeExample` class

```cpp
class EmissiveCubeExample : public ExampleRunner {
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
    vw::SkyPass *sky_pass;
    vw::IndirectLightPass *indirect_light;
    vw::ToneMappingPass *tone_mapping;

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

Scene creation (stays in this example, not shared):
- Ground plane: white colored material via `add_colored_mesh()`
- Cube: emissive textured material via `EmissiveTexturedMaterialHandler::create_material()` with stained glass texture at 50000.0f intensity
- Instance layout: scaled ground plane + elevated cube
- Camera at (5, 4, 5) looking at (0, 0.5, 0)
- Create UBO with camera matrices
- Upload meshes and build acceleration structures via `setup_ray_tracing()`
- Track initial texture states

Pipeline creation:
- Add all 6 passes: ZPass, DirectLightPass, AO, Sky, IndirectLight, ToneMapping
- Validate pipeline

### 4. `render()` implementation

- Create sky params: `SkyParameters::create_earth_sun(0.8f)`
- Read UBO data, extract camera position
- Configure all 6 passes:
  - zpass: set_uniform_buffer, set_scene
  - directLight: set_uniform_buffer, set_sky_parameters, set_camera_position, set_frame_count(indirectLight.get_frame_count())
  - aoPass: set_ao_radius(2.0f)
  - skyPass: set_sky_parameters, set_inverse_view_proj
  - indirectLight: set_sky_parameters
  - toneMapping: indirect_intensity=1.0, ACES, exposure=1.0, white_point=4.0, luminance_scale=10000.0
- Execute pipeline

### 5. `result_image()` implementation

Find ToneMapped slot in `tone_mapping->result_images()`.

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
    EmissiveCubeExample example(app);
    example.run();
}
```

## Files to Modify

### `examples/EmissiveCube/main.cpp`
- Complete rewrite: replace 305-line main() with ~120-line class + minimal main()

### `examples/EmissiveCube/CMakeLists.txt`
- Simplify link line:
```cmake
add_executable(EmissiveCube main.cpp)
target_link_libraries(EmissiveCube PRIVATE ExamplesCommon)
target_precompile_headers(EmissiveCube REUSE_FROM VulkanWrapperCoreLibrary)
```

## Dependencies

- Task 1 (ExampleRunner base class)
- `Common/SceneSetup.h` (for add_colored_mesh, create_ubo, setup_ray_tracing)

## Test Plan

1. Build: `cmake --build build --target EmissiveCube`
2. Run: `cd build/examples/EmissiveCube && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./EmissiveCube`
3. Verify `screenshot.png` is produced
4. Compare screenshot with pre-refactor version (should be pixel-identical)
5. Verify it screenshots after 256 AO samples
6. Verify indirect lighting is visible in the screenshot (emissive cube illuminating the ground plane)
