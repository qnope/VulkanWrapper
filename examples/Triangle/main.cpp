#include "Common/ExampleRunner.h"
#include <VulkanWrapper/Descriptors/Vertex.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>
#include <VulkanWrapper/Memory/Barrier.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Model/Primitive.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Shader/ShaderCompiler.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/ResourceTracker.h>
#include <VulkanWrapper/Vulkan/Queue.h>

struct TrianglePushConstants {
    glm::mat4 mvp;
    glm::vec3 color;
};

class TriangleExample : public ExampleRunner {
    using VertexBuffer =
        vw::Buffer<vw::FullVertex3D, false, vw::VertexBufferUsage>;
    using IndexBuffer =
        vw::Buffer<uint32_t, false, vw::IndexBufferUsage>;

    std::optional<VertexBuffer> m_vertex_buffer;
    std::optional<IndexBuffer> m_index_buffer;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
    TrianglePushConstants m_push_constants{};
    uint32_t m_index_count = 0;

  public:
    using ExampleRunner::ExampleRunner;

  protected:
    void setup() override {
        auto [vertices, indices] =
            vw::Model::create_triangle(
                vw::Model::PlanePrimitive::XY);
        m_index_count =
            static_cast<uint32_t>(indices.size());

        m_vertex_buffer.emplace(
            vw::allocate_vertex_buffer<vw::FullVertex3D, false>(
                *app().allocator, vertices.size()));
        m_index_buffer.emplace(
            vw::create_buffer<uint32_t, false,
                              vw::IndexBufferUsage>(
                *app().allocator, indices.size()));

        {
            vw::StagingBufferManager staging(app().device,
                                             app().allocator);
            staging.fill_buffer(
                std::span<const vw::FullVertex3D>{vertices},
                *m_vertex_buffer, 0);
            staging.fill_buffer(
                std::span<const uint32_t>{indices},
                *m_index_buffer, 0);
            auto cmd = staging.fill_command_buffer();
            app().device->graphicsQueue().enqueue_command_buffer(
                cmd);
            app().device->graphicsQueue().submit({}, {}, {}).wait();
        }

        vw::ShaderCompiler compiler;
        auto vert = compiler.compile_file_to_module(
            app().device,
            "../../../examples/Triangle/triangle.vert");
        auto frag = compiler.compile_file_to_module(
            app().device,
            "../../../examples/Triangle/triangle.frag");

        auto layout =
            vw::PipelineLayoutBuilder(app().device)
                .with_push_constant_range(
                    {vk::ShaderStageFlagBits::eAllGraphics, 0,
                     sizeof(TrianglePushConstants)})
                .build();

        m_pipeline =
            vw::GraphicsPipelineBuilder(app().device,
                                        std::move(layout))
                .add_shader(vk::ShaderStageFlagBits::eVertex,
                            vert)
                .add_shader(vk::ShaderStageFlagBits::eFragment,
                            frag)
                .add_vertex_binding<vw::FullVertex3D>()
                .add_color_attachment(app().swapchain.format())
                .with_dynamic_viewport_scissor()
                .build();

        auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f));
        float aspect =
            static_cast<float>(app().swapchain.width()) /
            static_cast<float>(app().swapchain.height());
        auto proj = glm::perspective(glm::radians(60.0f), aspect,
                                     0.1f, 10.0f);
        proj[1][1] *= -1; // Vulkan Y flip

        m_push_constants.mvp = proj * view;
        m_push_constants.color = glm::vec3(0.2f, 0.6f, 1.0f);
    }

    void render(vk::CommandBuffer cmd, vw::Transfer &transfer,
                uint32_t frame_index) override {
        const auto &image_view =
            app().swapchain.image_views()[frame_index];

        auto swapchain_width =
            static_cast<uint32_t>(app().swapchain.width());
        auto swapchain_height =
            static_cast<uint32_t>(app().swapchain.height());

        transfer.resourceTracker().request(
            vw::Barrier::ImageState{
                .image = image_view->image()->handle(),
                .subresourceRange =
                    image_view->subresource_range(),
                .layout =
                    vk::ImageLayout::eColorAttachmentOptimal,
                .stage = vk::PipelineStageFlagBits2::
                    eColorAttachmentOutput,
                .access = vk::AccessFlagBits2::
                    eColorAttachmentWrite});
        transfer.resourceTracker().flush(cmd);

        vk::RenderingAttachmentInfo color_attachment{};
        color_attachment.imageView = image_view->handle();
        color_attachment.imageLayout =
            vk::ImageLayout::eColorAttachmentOptimal;
        color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
        color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
        color_attachment.clearValue.color = vk::ClearColorValue{
            std::array{0.0f, 0.0f, 0.0f, 1.0f}};

        vk::RenderingInfo rendering_info{};
        rendering_info.renderArea = vk::Rect2D{
            {0, 0}, {swapchain_width, swapchain_height}};
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &color_attachment;

        cmd.beginRendering(rendering_info);

        vk::Viewport viewport{
            0.0f,
            0.0f,
            static_cast<float>(swapchain_width),
            static_cast<float>(swapchain_height),
            0.0f,
            1.0f};
        vk::Rect2D scissor{
            {0, 0}, {swapchain_width, swapchain_height}};

        cmd.setViewport(0, viewport);
        cmd.setScissor(0, scissor);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                         m_pipeline->handle());

        vk::Buffer vb = m_vertex_buffer->handle();
        vk::DeviceSize offset = 0;
        cmd.bindVertexBuffers(0, vb, offset);
        cmd.bindIndexBuffer(m_index_buffer->handle(), 0,
                            vk::IndexType::eUint32);

        cmd.pushConstants<TrianglePushConstants>(
            m_pipeline->layout().handle(),
            vk::ShaderStageFlagBits::eAllGraphics, 0,
            m_push_constants);

        cmd.drawIndexed(m_index_count, 1, 0, 0, 0);
        cmd.endRendering();
    }
};

int main() {
    App app;
    TriangleExample example(app);
    example.run();
}
