#include "DeferredRenderingManager.h"

#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/Allocator.h>

DeferredRenderingManager::DeferredRenderingManager(
    std::shared_ptr<vw::Device> device, std::shared_ptr<vw::Allocator> allocator,
    const vw::Swapchain &swapchain,
    const vw::Model::MeshManager &mesh_manager,
    const vw::Model::Scene &scene,
    const vw::Buffer<UBOData, true, vw::UniformBufferUsage> &uniform_buffer,
    vk::AccelerationStructureKHR tlas,
    Config config)
    : m_device{std::move(device)}
    , m_allocator{std::move(allocator)}
    , m_scene{scene}
    , m_config{std::move(config)}
    , m_tlas{tlas} {

    m_sampler = vw::SamplerBuilder(m_device).build();

    create_gbuffers(swapchain);
    create_uniform_descriptors(uniform_buffer);
    create_zpass_resources();
    create_color_pass_resources(mesh_manager);
    create_sun_light_pass_resources();
    create_sky_pass_resources();
    create_renderings();
}

void DeferredRenderingManager::create_gbuffers(const vw::Swapchain &swapchain) {
    constexpr auto usageFlags = vk::ImageUsageFlagBits::eColorAttachment |
                                vk::ImageUsageFlagBits::eInputAttachment |
                                vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferSrc;

    auto create_img = [&](vk::Format format, vk::ImageUsageFlags otherFlags = {}) {
        return m_allocator->create_image_2D(swapchain.width(), swapchain.height(),
                                            false, format, usageFlags | otherFlags);
    };

    auto create_img_view = [&](auto img) {
        return vw::ImageViewBuilder(m_device, img)
            .setImageType(vk::ImageViewType::e2D)
            .build();
    };

    for (int i = 0; i < swapchain.number_images(); ++i) {
        auto img_color = create_img(m_config.gbuffer_color_formats[0]);
        auto img_position = create_img(m_config.gbuffer_color_formats[1]);
        auto img_normal = create_img(m_config.gbuffer_color_formats[2]);
        auto img_tangeant = create_img(m_config.gbuffer_color_formats[3]);
        auto img_biTangeant = create_img(m_config.gbuffer_color_formats[4]);
        auto img_light =
            create_img(m_config.gbuffer_color_formats[5], vk::ImageUsageFlagBits::eStorage);

        auto img_depth = m_allocator->create_image_2D(
            swapchain.width(), swapchain.height(), false, m_config.depth_format,
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eSampled);

        m_gbuffers.push_back({create_img_view(img_color),
                              create_img_view(img_position),
                              create_img_view(img_normal),
                              create_img_view(img_tangeant),
                              create_img_view(img_biTangeant),
                              create_img_view(img_light),
                              create_img_view(img_depth)});
    }
}

void DeferredRenderingManager::create_uniform_descriptors(
    const vw::Buffer<UBOData, true, vw::UniformBufferUsage> &uniform_buffer) {
    m_uniform_descriptor_layout = create_zpass_descriptor_layout(m_device);
    m_uniform_descriptor_pool =
        vw::DescriptorPoolBuilder(m_device, m_uniform_descriptor_layout).build();
    m_uniform_descriptor_set =
        create_zpass_descriptor_set(*m_uniform_descriptor_pool, uniform_buffer);
}

void DeferredRenderingManager::create_zpass_resources() {
    m_zpass_pipeline = create_zpass_pipeline(m_device, m_config.depth_format,
                                             m_uniform_descriptor_layout);
}

void DeferredRenderingManager::create_color_pass_resources(
    const vw::Model::MeshManager &mesh_manager) {
    m_mesh_renderer =
        create_renderer(m_device, m_config.gbuffer_color_formats,
                        m_config.depth_format, mesh_manager, m_uniform_descriptor_layout);
}

void DeferredRenderingManager::create_sun_light_pass_resources() {
    m_sunlight_descriptor_layout = create_sun_light_pass_descriptor_layout(m_device);
    m_sunlight_descriptor_pool =
        vw::DescriptorPoolBuilder(m_device, m_sunlight_descriptor_layout).build();

    for (const auto &gBuffer : m_gbuffers) {
        m_sunlight_descriptor_sets.push_back(create_sun_light_pass_descriptor_set(
            *m_sunlight_descriptor_pool, m_sampler, gBuffer, m_tlas));
    }
}

void DeferredRenderingManager::create_sky_pass_resources() {
    // Sky pass uses the uniform descriptor layout directly
}

void DeferredRenderingManager::create_renderings() {
    int i = 0;
    for (const auto &gBuffer : m_gbuffers) {
        auto depth_subpass =
            std::make_shared<ZPass>(m_device, m_scene, m_uniform_descriptor_layout,
                                    *m_uniform_descriptor_set, gBuffer, m_zpass_pipeline);

        auto color_subpass = std::make_shared<ColorSubpass>(
            m_device, m_scene, m_uniform_descriptor_layout,
            *m_uniform_descriptor_set, gBuffer, m_mesh_renderer);

        auto sunlight_pass =
            create_sun_light_pass(m_device, m_sunlight_descriptor_layout,
                                  m_sunlight_descriptor_sets[i], gBuffer.light,
                                  &m_sun_angle);

        auto sky_pass = create_sky_pass(m_device, m_uniform_descriptor_layout,
                                        *m_uniform_descriptor_set, gBuffer.light,
                                        gBuffer.depth, &m_sun_angle);

        m_renderings.emplace_back(vw::RenderingBuilder()
                                      .add_subpass(depth_subpass)
                                      .add_subpass(color_subpass)
                                      .add_subpass(sunlight_pass)
                                      .add_subpass(sky_pass)
                                      .build());
        i++;
    }
}
