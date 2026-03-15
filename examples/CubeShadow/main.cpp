#include "Common/ExampleRunner.h"
#include "Common/SceneSetup.h"
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/RenderPass/DirectLightPass.h>
#include <VulkanWrapper/RenderPass/RenderPipeline.h>
#include <VulkanWrapper/RenderPass/SkyParameters.h>
#include <VulkanWrapper/RenderPass/SkyPass.h>
#include <VulkanWrapper/RenderPass/Slot.h>
#include <VulkanWrapper/RenderPass/ToneMappingPass.h>
#include <VulkanWrapper/RenderPass/ZPass.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>

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

  protected:
    void setup() override {
        auto scene_config =
            create_cube_plane_scene(m_mesh_manager);
        m_camera = scene_config.camera;

        float aspect =
            static_cast<float>(app().swapchain.width()) /
            static_cast<float>(app().swapchain.height());
        m_uniform_buffer =
            create_ubo(*app().allocator, aspect, m_camera);

        setup_ray_tracing(m_mesh_manager, m_ray_traced_scene,
                          *app().device,
                          scene_config.instances);

        for (const auto &resource :
             m_mesh_manager.material_manager()
                 .get_resources()) {
            transfer().resourceTracker().track(resource);
        }

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

        auto result = m_pipeline.validate();
        if (!result.valid) {
            for (const auto &error : result.errors) {
                std::cout << "Pipeline validation error: "
                          << error << std::endl;
            }
        }

        m_sky_params =
            vw::SkyParameters::create_earth_sun(45.0f);
        m_sky_params.star_direction = glm::normalize(
            glm::vec3(0.5f, -0.707f, 0.5f));
    }

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

    std::shared_ptr<const vw::ImageView>
    result_image() override {
        for (const auto &[slot, cached] :
             m_tone_mapping->result_images()) {
            if (slot == vw::Slot::ToneMapped)
                return cached.view;
        }
        return nullptr;
    }

    void on_resize(vw::Width width,
                   vw::Height height) override {
        m_pipeline.reset_accumulation();
        float new_aspect = static_cast<float>(width) /
                           static_cast<float>(height);
        auto ubo_data = UBOData::create(
            new_aspect, m_camera.view_matrix());
        m_uniform_buffer->write(ubo_data, 0);
    }
};

int main() {
    App app;
    CubeShadowExample example(app);
    example.run();
}
