#include "Common/ExampleRunner.h"
#include "Common/SceneSetup.h"
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/RenderPass/AmbientOcclusionPass.h>
#include <VulkanWrapper/RenderPass/DirectLightPass.h>
#include <VulkanWrapper/RenderPass/RenderPipeline.h>
#include <VulkanWrapper/RenderPass/SkyParameters.h>
#include <VulkanWrapper/RenderPass/Slot.h>
#include <VulkanWrapper/RenderPass/ZPass.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>

class AOExample : public ExampleRunner {
  public:
    explicit AOExample(App &app)
        : ExampleRunner(app),
          m_mesh_manager(app.device, app.allocator),
          m_ray_traced_scene(app.device, app.allocator) {}

  protected:
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

        auto result = m_pipeline.validate();
        if (!result.valid) {
            for (const auto &error : result.errors) {
                std::cout << "Pipeline validation error: "
                          << error << std::endl;
            }
        }
    }

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

    std::shared_ptr<const vw::ImageView>
    result_image() override {
        for (const auto &[slot, cached] :
             m_ao_pass->result_images()) {
            if (slot == vw::Slot::AmbientOcclusion)
                return cached.view;
        }
        return nullptr;
    }

    void on_resize(vw::Width width,
                   vw::Height height) override {
        m_pipeline.reset_accumulation();

        float aspect = static_cast<float>(width) /
                       static_cast<float>(height);
        auto ubo_data =
            UBOData::create(aspect, m_camera.view_matrix());
        m_uniform_buffer->write(ubo_data, 0);
    }

    bool should_screenshot() const override {
        return m_ao_pass->get_frame_count() == 256;
    }

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

int main() {
    App app;
    AOExample example(app);
    example.run();
}
