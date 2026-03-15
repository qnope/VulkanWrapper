---
sidebar_position: 3
---

# 3. Ambient Occlusion

In the [previous tutorial](cube-shadow.md) you rendered a cube with direct sunlight and ray-traced shadows. That scene looks good, but the cube seems to float slightly above the ground because there is no soft darkening where the two surfaces meet. **Ambient Occlusion** (AO) adds that subtle contact shadow by estimating how much ambient light can reach each point.

![Ambient occlusion visualization showing subtle darkening around the cube base](/img/tutorials/ambient-occlusion.png)

## What is ambient occlusion?

Ambient occlusion measures how "enclosed" a surface point is. Points inside corners, crevices, or close to other geometry receive less ambient light and appear darker. VulkanWrapper's `AmbientOcclusionPass` computes this using **ray tracing**: for each pixel it shoots random rays into the hemisphere above the surface and checks how many hit nearby geometry.

Because the rays are random, a single sample is noisy. The pass uses **progressive accumulation** -- each frame adds more samples and the result converges to a smooth image over time.

## Building on CubeShadow

The AmbientOcclusion example (`examples/AmbientOcclusion/main.cpp`) uses the same scene as CubeShadow. The key difference is the pass lineup: we replace the sky and tone mapping passes with an `AmbientOcclusionPass` and display its raw output directly.

The pipeline for this tutorial:

| Pass | Purpose |
|------|---------|
| ZPass | Rasterize geometry into the G-buffer |
| DirectLightPass | Compute direct lighting with ray-traced shadows |
| AmbientOcclusionPass | Trace ambient rays to estimate occlusion |

We display the AO buffer directly (without sky or tone mapping) to see exactly what the pass produces.

## Step 1 -- Scene setup (same as CubeShadow)

The scene creation code is identical to the previous tutorial:

```cpp
void setup() override {
    auto scene_config =
        create_cube_plane_scene(m_mesh_manager);
    m_camera = scene_config.camera;

    float aspect =
        static_cast<float>(app().swapchain.width()) /
        static_cast<float>(app().swapchain.height());
    m_uniform_buffer.emplace(
        create_ubo(*app().allocator, aspect, m_camera));

    setup_ray_tracing(m_mesh_manager, m_ray_traced_scene,
                      *app().device,
                      scene_config.instances);

    for (const auto &resource :
         m_mesh_manager.material_manager().get_resources()) {
        transfer().resourceTracker().track(resource);
    }
```

## Step 2 -- Add AmbientOcclusionPass to the pipeline

After creating the ZPass and DirectLightPass (same as before), add the AO pass:

```cpp
    const std::filesystem::path shader_dir =
        "../../../VulkanWrapper/Shaders";

    m_zpass = &m_pipeline.add(std::make_unique<vw::ZPass>(
        app().device, app().allocator, shader_dir));

    m_direct_light =
        &m_pipeline.add(std::make_unique<vw::DirectLightPass>(
            app().device, app().allocator, shader_dir,
            m_ray_traced_scene,
            m_mesh_manager.material_manager()));

    m_ao_pass = &m_pipeline.add(
        std::make_unique<vw::AmbientOcclusionPass>(
            app().device, app().allocator, shader_dir,
            m_ray_traced_scene.tlas_handle()));
```

Notice that `AmbientOcclusionPass` takes `m_ray_traced_scene.tlas_handle()` -- it needs the TLAS to trace ambient rays against the scene geometry.

After adding all passes, validate as before:

```cpp
    auto result = m_pipeline.validate();
    if (!result.valid) {
        for (const auto &error : result.errors) {
            std::cout << "Pipeline validation error: "
                      << error << std::endl;
        }
    }
}
```

## Step 3 -- Configure per-frame parameters

The render loop sets parameters for each pass. The AO pass has one key parameter -- the radius:

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

    m_direct_light->set_uniform_buffer(*m_uniform_buffer);
    m_direct_light->set_sky_parameters(sky_params);
    m_direct_light->set_camera_position(camera_pos);
    m_direct_light->set_frame_count(0);

    m_ao_pass->set_ao_radius(2.0f);

    m_pipeline.execute(cmd, transfer.resourceTracker(),
                       app().swapchain.width(),
                       app().swapchain.height(),
                       frame_index);
}
```

`set_ao_radius(2.0f)` controls how far the ambient rays travel. A larger radius produces broader, softer shadows. A smaller radius produces tighter contact shadows. A value of 2.0 world units works well for this scene.

## Step 4 -- Display the AO result

Instead of displaying the tone-mapped output, we show the raw AO buffer:

```cpp
std::shared_ptr<const vw::ImageView>
result_image() override {
    for (const auto &[slot, cached] :
         m_ao_pass->result_images()) {
        if (slot == vw::Slot::AmbientOcclusion)
            return cached.view;
    }
    return nullptr;
}
```

This displays the AO values directly: white means fully lit, darker means more occluded. You will see darkening around the base of the cube where it sits on the ground plane.

## Step 5 -- Progressive accumulation and screenshots

AO is computed progressively. Each frame adds new random samples that are averaged with previous frames. After enough frames the noise smooths out. The example takes a screenshot after 256 accumulated frames:

```cpp
bool should_screenshot() const override {
    return m_ao_pass->get_frame_count() == 256;
}
```

`get_frame_count()` returns the number of accumulated samples. This counter resets to zero whenever you call `m_pipeline.reset_accumulation()` (for example, on window resize):

```cpp
void on_resize(vw::Width width,
               vw::Height height) override {
    m_pipeline.reset_accumulation();

    float aspect = static_cast<float>(width) /
                   static_cast<float>(height);
    auto ubo_data =
        UBOData::create(aspect, m_camera.view_matrix());
    m_uniform_buffer->write(ubo_data, 0);
}
```

## Complete class

```cpp
class AOExample : public ExampleRunner {
  public:
    explicit AOExample(App &app)
        : ExampleRunner(app),
          m_mesh_manager(app.device, app.allocator),
          m_ray_traced_scene(app.device, app.allocator) {}

  protected:
    // setup(), render(), result_image(),
    // on_resize(), should_screenshot() as shown above

  private:
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
};
```

## Understanding the AO output

The screenshot shows a grayscale image:

- **White areas** (value 1.0) are fully exposed to ambient light -- the open parts of the ground plane and the top of the cube.
- **Dark areas** (values approaching 0.0) are where geometry blocks ambient rays -- most noticeably at the seam where the cube meets the ground.

In a full rendering pipeline, this AO value would be multiplied with the ambient/indirect lighting term to darken enclosed areas. You will see this in the [next tutorial](emissive-cube.md).

## What you learned

- **AmbientOcclusionPass** traces random rays into the hemisphere above each surface point to estimate occlusion.
- **`set_ao_radius()`** controls the distance of ambient rays, trading between tight contact shadows and broad soft shadows.
- **Progressive accumulation** averages samples over many frames to reduce noise. Use `get_frame_count()` to check progress and `reset_accumulation()` when the scene changes.
- The pass outputs to `Slot::AmbientOcclusion`, which downstream passes can consume.

## Next steps

In the [next tutorial](emissive-cube.md), you will build the full rendering pipeline with **emissive materials**, **indirect lighting**, sky, and tone mapping -- combining all the passes into a complete physically-based renderer.
