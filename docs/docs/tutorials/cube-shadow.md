---
sidebar_position: 2
---

# 2. Cube with Shadows

In the [previous tutorial](triangle.md) you built a graphics pipeline by hand and rendered a triangle. Real scenes need multiple rendering passes -- depth, lighting, sky, tone mapping -- and managing them manually is tedious. This tutorial introduces the **RenderPipeline** system, **MeshManager**, and **RayTracedScene** to render a cube on a ground plane with ray-traced shadows and atmospheric sky.

![Cube on a white ground plane with shadow and atmospheric sky](/img/tutorials/cube-shadow.png)

## Overview

The CubeShadow example (`examples/CubeShadow/main.cpp`) uses four rendering passes chained together:

1. **ZPass** -- rasterizes the scene to produce depth, albedo, normals, tangents, and position buffers (the G-buffer).
2. **DirectLightPass** -- computes direct sun lighting with ray-traced shadows.
3. **SkyPass** -- renders an atmospheric sky in areas not covered by geometry.
4. **ToneMappingPass** -- converts HDR radiance values to displayable sRGB.

Each pass reads from and writes to named **slots** (like `Slot::Depth`, `Slot::Albedo`, `Slot::DirectLight`). The `RenderPipeline` automatically wires outputs of one pass to inputs of the next.

## Step 1 -- Set up the scene with MeshManager

`MeshManager` handles mesh storage, material assignment, and GPU upload. The helper function `create_cube_plane_scene` creates a white ground plane and a reddish cube:

```cpp
auto scene_config =
    create_cube_plane_scene(m_mesh_manager);
m_camera = scene_config.camera;
```

Under the hood, `create_cube_plane_scene` calls `add_colored_mesh` for each object:

```cpp
auto plane_index = add_colored_mesh(
    mesh_manager,
    vw::Model::create_square(vw::Model::PlanePrimitive::XZ),
    glm::vec3(1.0f));

auto cube_index = add_colored_mesh(
    mesh_manager, vw::Model::create_cube(),
    glm::vec3(0.7f, 0.3f, 0.3f));
```

Each mesh gets a `ColoredMaterialHandler` material -- a simple solid-color material. The function also sets up instance transforms (scaling the ground 20x and elevating the cube) and a camera position.

## Step 2 -- Create the uniform buffer

The camera matrices live in a host-visible uniform buffer so we can update them when the window resizes:

```cpp
float aspect =
    static_cast<float>(app().swapchain.width()) /
    static_cast<float>(app().swapchain.height());
m_uniform_buffer =
    create_ubo(*app().allocator, aspect, m_camera);
```

`create_ubo` builds a `Buffer<UBOData, true, UniformBufferUsage>` and fills it with projection, view, and inverse-view-projection matrices. The `true` template parameter makes it host-visible so `write()` works directly without staging.

## Step 3 -- Build ray tracing acceleration structures

Ray-traced shadows require a **Top-Level Acceleration Structure** (TLAS) built over the scene geometry:

```cpp
setup_ray_tracing(m_mesh_manager, m_ray_traced_scene,
                  *app().device,
                  scene_config.instances);
```

This helper does four things:

1. Uploads mesh vertex/index data to the GPU via `mesh_manager.fill_command_buffer()`.
2. Adds each mesh instance (with its transform) to the `RayTracedScene`.
3. Sets per-material shader binding table (SBT) offsets so the ray tracing pipeline knows which material to evaluate on hit.
4. Builds the BLAS (per-mesh) and TLAS (whole scene) acceleration structures.

After this call, the material manager's GPU resources (textures, buffers) also need to be tracked for barrier management:

```cpp
for (const auto &resource :
     m_mesh_manager.material_manager()
         .get_resources()) {
    transfer().resourceTracker().track(resource);
}
```

## Step 4 -- Assemble the render pipeline

Each pass is created individually and added to the pipeline with `add()`, which returns a reference you can use later to set per-frame parameters:

```cpp
const std::filesystem::path shader_dir =
    "../../../VulkanWrapper/Shaders";

m_zpass = &m_pipeline.add(
    std::make_unique<vw::ZPass>(
        app().device, app().allocator, shader_dir));

m_direct_light = &m_pipeline.add(
    std::make_unique<vw::DirectLightPass>(
        app().device, app().allocator, shader_dir,
        m_ray_traced_scene,
        m_mesh_manager.material_manager()));

m_sky_pass = &m_pipeline.add(
    std::make_unique<vw::SkyPass>(
        app().device, app().allocator, shader_dir));

m_tone_mapping = &m_pipeline.add(
    std::make_unique<vw::ToneMappingPass>(
        app().device, app().allocator, shader_dir,
        vk::Format::eB8G8R8A8Srgb, false));
```

The order matters -- passes execute in the order they are added, and each pass declares which slots it reads and writes. After adding all passes, validate the pipeline to check for missing connections:

```cpp
auto result = m_pipeline.validate();
if (!result.valid) {
    for (const auto &error : result.errors) {
        std::cout << "Pipeline validation error: "
                  << error << std::endl;
    }
}
```

## Step 5 -- Configure sky parameters

The atmospheric sky model is configured through `SkyParameters`:

```cpp
m_sky_params =
    vw::SkyParameters::create_earth_sun(45.0f);
m_sky_params.star_direction = glm::normalize(
    glm::vec3(0.5f, -0.707f, 0.5f));
```

`create_earth_sun(45.0f)` creates physically-based atmospheric parameters with the sun at 45 degrees above the horizon. You can adjust `star_direction` to change the sun's azimuth direction.

## Step 6 -- Render each frame

In `render()`, each pass is configured with up-to-date per-frame data, then the entire pipeline executes in one call:

```cpp
void render(vk::CommandBuffer cmd,
            vw::Transfer &transfer,
            uint32_t frame_index) override {
    auto ubo_data =
        m_uniform_buffer->read_as_vector(0, 1)[0];
    glm::vec3 camera_pos =
        glm::vec3(glm::inverse(ubo_data.view)[3]);

    m_zpass->set_uniform_buffer(*m_uniform_buffer);
    m_zpass->set_scene(m_ray_traced_scene);

    m_direct_light->set_uniform_buffer(
        *m_uniform_buffer);
    m_direct_light->set_sky_parameters(m_sky_params);
    m_direct_light->set_camera_position(camera_pos);
    m_direct_light->set_frame_count(0);

    m_sky_pass->set_sky_parameters(m_sky_params);
    m_sky_pass->set_inverse_view_proj(
        ubo_data.inverseViewProj);

    m_tone_mapping->set_operator(
        vw::ToneMappingOperator::ACES);
    m_tone_mapping->set_exposure(1.0f);
    m_tone_mapping->set_white_point(4.0f);
    m_tone_mapping->set_luminance_scale(10000.0f);
    m_tone_mapping->set_indirect_intensity(0.0f);

    m_pipeline.execute(cmd, transfer.resourceTracker(),
                       app().swapchain.width(),
                       app().swapchain.height(),
                       frame_index);
}
```

Each pass setter follows a consistent pattern: provide the data the pass needs before calling `execute()`. The pipeline then runs all passes in sequence, automatically handling image transitions between them.

### What each pass does per frame

| Pass | Inputs | Output Slot | Description |
|------|--------|-------------|-------------|
| ZPass | UBO, scene | Depth, Albedo, Normal, Tangent, Bitangent, Position | Rasterizes geometry into the G-buffer |
| DirectLightPass | UBO, G-buffer, sky params, TLAS | DirectLight | Traces shadow rays from each pixel toward the sun |
| SkyPass | Sky params, inverse VP matrix, depth | Sky | Renders atmospheric scattering for sky pixels |
| ToneMappingPass | DirectLight, Sky | ToneMapped | Applies ACES tone mapping to produce sRGB output |

## Step 7 -- Present the result

Override `result_image()` to tell `ExampleRunner` which image to blit to the swapchain:

```cpp
std::shared_ptr<const vw::ImageView>
result_image() override {
    for (const auto &[slot, cached] :
         m_tone_mapping->result_images()) {
        if (slot == vw::Slot::ToneMapped)
            return cached.view;
    }
    return nullptr;
}
```

## Step 8 -- Handle window resize

When the window is resized, accumulated data becomes invalid and the projection matrix needs updating:

```cpp
void on_resize(vw::Width width,
               vw::Height height) override {
    m_pipeline.reset_accumulation();
    float new_aspect = static_cast<float>(width) /
                       static_cast<float>(height);
    auto ubo_data = UBOData::create(
        new_aspect, m_camera.view_matrix());
    m_uniform_buffer->write(ubo_data, 0);
}
```

`reset_accumulation()` tells all passes to discard any progressively accumulated data and start fresh.

## Class structure

The member variables reflect the components we built:

```cpp
class CubeShadowExample : public ExampleRunner {
    vw::Model::MeshManager m_mesh_manager;
    vw::rt::RayTracedScene m_ray_traced_scene;
    CameraConfig m_camera;
    std::optional<
        vw::Buffer<UBOData, true, vw::UniformBufferUsage>>
        m_uniform_buffer;
    vw::RenderPipeline m_pipeline;
    vw::SkyParameters m_sky_params;

    vw::ZPass *m_zpass = nullptr;
    vw::DirectLightPass *m_direct_light = nullptr;
    vw::SkyPass *m_sky_pass = nullptr;
    vw::ToneMappingPass *m_tone_mapping = nullptr;

  public:
    explicit CubeShadowExample(App &app)
        : ExampleRunner(app),
          m_mesh_manager(app.device, app.allocator),
          m_ray_traced_scene(app.device, app.allocator) {}
    // ... setup(), render(), result_image(), on_resize()
};
```

The raw pointers to passes are non-owning -- the `RenderPipeline` owns the passes through `std::unique_ptr`. The pointers are only used to call setters each frame.

## What you learned

- **MeshManager** stores meshes and their materials, and uploads everything to the GPU in a single batch.
- **RayTracedScene** builds BLAS/TLAS acceleration structures for ray-traced effects.
- **RenderPipeline** chains rendering passes together and validates their slot connections.
- **Passes** (ZPass, DirectLightPass, SkyPass, ToneMappingPass) are composable building blocks. Each reads from and writes to named slots.
- **SkyParameters** configures physically-based atmospheric rendering.
- The pipeline handles all image allocations and layout transitions internally.

## Next steps

This scene has direct sunlight but no soft ambient lighting. In the [next tutorial](ambient-occlusion.md), you will add **AmbientOcclusionPass** to simulate how nearby geometry blocks ambient light.
