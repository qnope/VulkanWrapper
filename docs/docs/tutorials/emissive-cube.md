---
sidebar_position: 4
---

# 4. Emissive Materials and Global Illumination

In the previous tutorials you built up a rendering pipeline piece by piece: geometry, direct lighting, sky, tone mapping, and ambient occlusion. This final tutorial brings everything together and adds two new features: **emissive materials** that glow with light, and **IndirectLightPass** that traces rays to simulate how that light bounces around the scene.

![Emissive stained glass cube illuminating the dark ground with colorful global illumination](/img/tutorials/emissive-cube.png)

## What makes this example different

The EmissiveCube example (`examples/EmissiveCube/main.cpp`) uses the most complete rendering pipeline in VulkanWrapper. Compared to the CubeShadow example:

- The cube uses an **emissive textured material** instead of a flat color. It emits light based on a stained glass texture.
- An **IndirectLightPass** traces secondary rays that bounce off surfaces, picking up emitted light and carrying it to other surfaces (global illumination).
- **AmbientOcclusionPass** darkens enclosed areas.
- **ToneMappingPass** now includes indirect light intensity, blending the indirect illumination into the final image.

The full pass lineup:

| Order | Pass | What it adds |
|-------|------|-------------|
| 1 | ZPass | G-buffer (depth, albedo, normals, position) |
| 2 | DirectLightPass | Direct sunlight with ray-traced shadows |
| 3 | AmbientOcclusionPass | Ambient occlusion |
| 4 | SkyPass | Atmospheric sky |
| 5 | IndirectLightPass | Bounced light from emissive surfaces |
| 6 | ToneMappingPass | HDR to sRGB conversion |

## Step 1 -- Create the ground plane

Instead of using the shared `create_cube_plane_scene` helper, this example builds the scene manually because it needs a custom material for the cube. The ground plane uses a gray colored material:

```cpp
auto plane_index = add_colored_mesh(
    m_mesh_manager,
    vw::Model::create_square(
        vw::Model::PlanePrimitive::XZ),
    glm::vec3(0.8f));
```

## Step 2 -- Create the emissive cube

The cube gets an **emissive textured material**. This material type emits light proportional to the texture color and an intensity value:

```cpp
auto cube_primitives = vw::Model::create_cube();
auto *emissive_handler = static_cast<
    vw::Model::Material::
        EmissiveTexturedMaterialHandler *>(
    m_mesh_manager.material_manager().handler(
        vw::Model::Material::
            emissive_textured_material_tag));
auto emissive_material =
    emissive_handler->create_material(
        "../../../Images/stained.png", 50000.0f);
m_mesh_manager.add_mesh(
    std::move(cube_primitives.vertices),
    std::move(cube_primitives.indices),
    emissive_material);
auto cube_index =
    m_mesh_manager.meshes().size() - 1;
```

Key points:

- **`emissive_textured_material_tag`** identifies the emissive textured material type in the material system. Each material type has a unique tag.
- **`handler()`** returns the handler for that material type, which you cast to the concrete type to call `create_material()`.
- **`create_material("stained.png", 50000.0f)`** loads the texture and sets the emissive intensity. The high intensity value (50,000) makes the cube a strong light source -- it needs to be high because the value represents physical luminance in candelas per square meter.

## Step 3 -- Position the objects

The scene layout uses the same pattern as before: a scaled ground plane and an elevated cube, with instance transforms:

```cpp
std::vector<PendingInstance> instances;
instances.push_back(
    {plane_index,
     glm::scale(glm::mat4(1.0f),
                glm::vec3(20.0f))});
instances.push_back(
    {cube_index,
     glm::translate(glm::mat4(1.0f),
                    glm::vec3(0.0f, 1.5f, 0.0f))});

m_camera = CameraConfig{
    .eye = glm::vec3(5.0f, 4.0f, 5.0f),
    .target = glm::vec3(0.0f, 0.5f, 0.0f),
};
```

Then the uniform buffer and ray tracing structures are created exactly as in the previous tutorials:

```cpp
float aspect =
    static_cast<float>(app().swapchain.width()) /
    static_cast<float>(app().swapchain.height());
m_uniform_buffer =
    create_ubo(*app().allocator, aspect, m_camera);

setup_ray_tracing(m_mesh_manager, m_ray_traced_scene,
                  *app().device, instances);

for (const auto &resource :
     m_mesh_manager.material_manager()
         .get_resources()) {
    transfer().resourceTracker().track(resource);
}
```

## Step 4 -- Build the full pipeline

This is the most complete pipeline configuration. Six passes, each building on the output of previous ones:

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

m_ao_pass = &m_pipeline.add(
    std::make_unique<vw::AmbientOcclusionPass>(
        app().device, app().allocator, shader_dir,
        m_ray_traced_scene.tlas_handle()));

m_sky_pass = &m_pipeline.add(
    std::make_unique<vw::SkyPass>(
        app().device, app().allocator, shader_dir));

m_indirect_light = &m_pipeline.add(
    std::make_unique<vw::IndirectLightPass>(
        app().device, app().allocator, shader_dir,
        m_ray_traced_scene.tlas(),
        m_ray_traced_scene.geometry_buffer(),
        m_mesh_manager.material_manager()));

m_tone_mapping = &m_pipeline.add(
    std::make_unique<vw::ToneMappingPass>(
        app().device, app().allocator, shader_dir));
```

### IndirectLightPass

`IndirectLightPass` is the new pass in this tutorial. It needs:

- **`tlas()`** -- the top-level acceleration structure for ray tracing.
- **`geometry_buffer()`** -- a buffer containing per-instance vertex/index data so the shader can look up surface properties at ray hit points.
- **`material_manager()`** -- to evaluate material properties (especially emissive values) at hit points.

The pass traces secondary rays from each visible surface point. When a ray hits an emissive surface, it carries that light back to the origin point. Over many frames of progressive accumulation, this produces smooth color bleeding from the emissive cube onto the ground.

## Step 5 -- Configure per-frame parameters

The render loop feeds data to all six passes:

```cpp
void render(vk::CommandBuffer cmd,
            vw::Transfer &transfer,
            uint32_t frame_index) override {
    auto sky_params =
        vw::SkyParameters::create_earth_sun(0.8f);
    auto ubo_data =
        m_uniform_buffer->read_as_vector(0, 1)[0];
    glm::vec3 camera_pos =
        glm::vec3(glm::inverse(ubo_data.view)[3]);

    m_zpass->set_uniform_buffer(*m_uniform_buffer);
    m_zpass->set_scene(m_ray_traced_scene);

    m_direct_light->set_uniform_buffer(
        *m_uniform_buffer);
    m_direct_light->set_sky_parameters(sky_params);
    m_direct_light->set_camera_position(camera_pos);
    m_direct_light->set_frame_count(
        m_indirect_light->get_frame_count());

    m_ao_pass->set_ao_radius(2.0f);

    m_sky_pass->set_sky_parameters(sky_params);
    m_sky_pass->set_inverse_view_proj(
        ubo_data.inverseViewProj);

    m_indirect_light->set_sky_parameters(sky_params);

    m_tone_mapping->set_indirect_intensity(1.0f);
    m_tone_mapping->set_operator(
        vw::ToneMappingOperator::ACES);
    m_tone_mapping->set_exposure(1.0f);
    m_tone_mapping->set_white_point(4.0f);
    m_tone_mapping->set_luminance_scale(10000.0f);

    m_pipeline.execute(cmd, transfer.resourceTracker(),
                       app().swapchain.width(),
                       app().swapchain.height(),
                       frame_index);
}
```

Two things to notice compared to the CubeShadow example:

1. **`m_direct_light->set_frame_count(m_indirect_light->get_frame_count())`** -- the direct light pass uses the indirect light's frame count to synchronize progressive accumulation. Both passes accumulate samples together so the final image converges uniformly.

2. **`m_tone_mapping->set_indirect_intensity(1.0f)`** -- in CubeShadow this was 0.0 (no indirect light). Setting it to 1.0 tells the tone mapping pass to blend indirect illumination at full strength into the final image.

## Step 6 -- Result and screenshot

The result comes from the tone mapping pass, same as CubeShadow:

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

The screenshot is taken after 256 accumulated samples using the AO pass's frame counter:

```cpp
bool should_screenshot() const override {
    return m_ao_pass->get_frame_count() == 256;
}
```

## Understanding the result

The final image shows several effects working together:

- **Direct sunlight** illuminates the top and sides of the cube and the ground plane, with a sharp shadow cast by the cube.
- **Atmospheric sky** fills the background with a realistic gradient.
- **Ambient occlusion** darkens the ground where it meets the cube.
- **Indirect illumination** from the emissive stained glass texture bleeds colorful light onto the surrounding ground. This is the global illumination contribution -- light that has bounced off the emissive cube surface.
- **Tone mapping** (ACES) compresses the wide HDR range into displayable sRGB values.

## Complete class

```cpp
class EmissiveCubeExample : public ExampleRunner {
    vw::Model::MeshManager m_mesh_manager;
    vw::rt::RayTracedScene m_ray_traced_scene;
    CameraConfig m_camera;
    std::optional<
        vw::Buffer<UBOData, true, vw::UniformBufferUsage>>
        m_uniform_buffer;
    vw::RenderPipeline m_pipeline;

    vw::ZPass *m_zpass = nullptr;
    vw::DirectLightPass *m_direct_light = nullptr;
    vw::AmbientOcclusionPass *m_ao_pass = nullptr;
    vw::SkyPass *m_sky_pass = nullptr;
    vw::IndirectLightPass *m_indirect_light = nullptr;
    vw::ToneMappingPass *m_tone_mapping = nullptr;

  public:
    explicit EmissiveCubeExample(App &app)
        : ExampleRunner(app),
          m_mesh_manager(app.device, app.allocator),
          m_ray_traced_scene(app.device, app.allocator) {}
    // ... setup(), render(), result_image(),
    //     on_resize(), should_screenshot()
};
```

## How the passes connect

Here is the complete data flow through the pipeline:

```
ZPass
  writes: Depth, Albedo, Normal, Tangent, Bitangent, Position
     |
     v
DirectLightPass
  reads:  Depth, Albedo, Normal, Position (from ZPass)
  writes: DirectLight
     |
     v
AmbientOcclusionPass
  reads:  Depth, Normal, Position (from ZPass)
  writes: AmbientOcclusion
     |
     v
SkyPass
  reads:  Depth (from ZPass)
  writes: Sky
     |
     v
IndirectLightPass
  reads:  Depth, Normal, Position (from ZPass)
  writes: IndirectLight
     |
     v
ToneMappingPass
  reads:  DirectLight, Sky, AmbientOcclusion, IndirectLight
  writes: ToneMapped
```

Each pass only reads from slots that previous passes have written. The `RenderPipeline` validates these connections at setup time so you get clear error messages if something is missing.

## What you learned

- **EmissiveTexturedMaterialHandler** creates materials that emit light based on a texture and intensity value.
- **IndirectLightPass** traces secondary rays to simulate global illumination -- light bouncing from emissive (or illuminated) surfaces onto others.
- **Progressive accumulation** applies to all ray-traced passes. Both direct and indirect lighting converge together over many frames.
- **ToneMappingPass** blends direct light, indirect light, sky, and AO into the final image. The `set_indirect_intensity()` parameter controls how much indirect illumination contributes.
- The full pipeline (ZPass, DirectLightPass, AmbientOcclusionPass, SkyPass, IndirectLightPass, ToneMappingPass) is a complete physically-based renderer built from composable passes.

## Where to go from here

You have now seen all the major components of VulkanWrapper's rendering system. Some ideas for experimentation:

- **Change the emissive intensity** -- try values from 1,000 to 100,000 and observe how the light bleeding changes.
- **Add more objects** -- create additional meshes with `create_cube()` or `create_square()` and add them as instances with different transforms.
- **Adjust the AO radius** -- a smaller radius (0.5) produces tight contact shadows; a larger radius (5.0) produces broader darkening.
- **Change the sky** -- try different sun angles with `SkyParameters::create_earth_sun()` to simulate sunrise, noon, or sunset.
- **Mix material types** -- combine colored, textured, and emissive materials in the same scene.
