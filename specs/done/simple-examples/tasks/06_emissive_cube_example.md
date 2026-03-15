# Task 6: EmissiveCube Example

## Summary

Implement `examples/EmissiveCube/`, the same cube+plane scene but with the cube using an emissive textured material (`Images/stained.png`). Full deferred pipeline with indirect lighting: `ZPass -> DirectLightPass -> AmbientOcclusionPass -> SkyPass -> IndirectLightPass -> ToneMappingPass`.

## Dependencies

- **Task 1** (Primitive Geometry Library): For geometry generation.
- **Task 2** (Common Scene Setup): For scene helpers and UBO.

## Implementation Steps

### 1. Create `examples/EmissiveCube/main.cpp`

**Scene setup differs from CubeShadow** — the cube uses `EmissiveTexturedMaterialHandler` instead of `ColoredMaterialHandler`:

```cpp
// Create ground plane with colored material (white)
auto plane_primitives = vw::Model::create_square(vw::Model::PlanePrimitive::XZ);
auto plane_index = add_colored_mesh(mesh_manager, std::move(plane_primitives),
                                     glm::vec3(0.8f));

// Create cube with emissive textured material
auto cube_primitives = vw::Model::create_cube();

auto *emissive_handler =
    static_cast<vw::Model::Material::EmissiveTexturedMaterialHandler *>(
        mesh_manager.material_manager().handler(
            vw::Model::Material::emissive_textured_material_tag));

auto emissive_material = emissive_handler->create_material(
    "../../../Images/stained.png", 50000.0f);

mesh_manager.add_mesh(std::move(cube_primitives.vertices),
                       std::move(cube_primitives.indices),
                       emissive_material);
auto cube_index = mesh_manager.meshes().size() - 1;
```

The rest of instance creation, transforms, and camera follow the same pattern as `create_cube_plane_scene()` but are done inline since the material setup is different.

**Full render pipeline:**
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
auto &skyPass = pipeline.add(
    std::make_unique<vw::SkyPass>(app.device, app.allocator, shader_dir));
auto &indirectLight = pipeline.add(
    std::make_unique<vw::IndirectLightPass>(
        app.device, app.allocator, shader_dir, ray_traced_scene.tlas(),
        ray_traced_scene.geometry_buffer(),
        mesh_manager.material_manager()));
auto &toneMapping = pipeline.add(
    std::make_unique<vw::ToneMappingPass>(
        app.device, app.allocator, shader_dir));

pipeline.validate();
```

**Per-frame configuration** (all passes):
```cpp
// ZPass
zpass.set_uniform_buffer(uniform_buffer);
zpass.set_scene(ray_traced_scene);

// DirectLight
directLight.set_uniform_buffer(uniform_buffer);
directLight.set_sky_parameters(sky_params);
directLight.set_camera_position(camera_pos);
directLight.set_frame_count(indirectLight.get_frame_count());

// AO
aoPass.set_ao_radius(2.0f);

// Sky
skyPass.set_sky_parameters(sky_params);
skyPass.set_inverse_view_proj(ubo_data.inverseViewProj);

// IndirectLight — no per-frame config needed beyond what's set at construction
indirectLight.set_sky_parameters(sky_params);

// ToneMapping
toneMapping.set_indirect_intensity(1.0f);
toneMapping.set_operator(vw::ToneMappingOperator::ACES);
toneMapping.set_exposure(1.0f);
toneMapping.set_white_point(4.0f);
toneMapping.set_luminance_scale(10000.0f);
```

**Output**: Blit tone-mapped result to swapchain (same as CubeShadow/Advanced pattern).

**Screenshot timing**: Indirect light is progressive — wait for 32 frames (same as `aoPass.get_frame_count() == 32`).

### 2. Verify `Images/stained.png` exists

The emissive texture `Images/stained.png` is already used by the Advanced example. Verify it exists at the repository root. The runtime path from `build/examples/EmissiveCube/` is `../../../Images/stained.png`.

### 3. Create `examples/EmissiveCube/CMakeLists.txt`

```cmake
add_executable(EmissiveCube main.cpp)
target_link_libraries(EmissiveCube PRIVATE App ExamplesCommon)
target_precompile_headers(EmissiveCube REUSE_FROM VulkanWrapperCoreLibrary)
```

### 4. Register in `examples/CMakeLists.txt`

Add `add_subdirectory(EmissiveCube)`.

## Design Considerations

- **Emissive intensity**: The `stained.png` texture with intensity 50000.0f should create visible colored light on the ground plane. This value may need tuning — the Advanced example uses 300000.0f but that's for Sponza scale. For unit-sized geometry scaled ~1-20x, a lower intensity is appropriate.
- **IndirectLightPass construction**: Requires `ray_traced_scene.tlas()` (the TLAS object, not just the handle), `ray_traced_scene.geometry_buffer()`, and the material manager. These provide the geometry + material data for closest-hit shaders during indirect bounce computation.
- **Material SBT mapping**: Must be set on the `RayTracedScene` before building acceleration structures: `ray_traced_scene.set_material_sbt_mapping(mesh_manager.material_manager().sbt_mapping())`. This is handled by the `setup_ray_tracing()` helper from Common/SceneSetup.h, but since EmissiveCube has custom material setup, ensure this is called after all materials are registered.

## Test Plan

- **Build validation**: `cmake --build build --target EmissiveCube` compiles.
- **Visual validation**: `screenshot.png` shows an emissive cube (stained glass texture) illuminating the surrounding white ground plane with colored indirect light. The ground near the cube should show color bleeding from the emissive texture.
