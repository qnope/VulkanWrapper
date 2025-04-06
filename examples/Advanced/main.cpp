#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Command/CommandBuffer.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/Image/CombinedImage.h>
#include <VulkanWrapper/Image/Framebuffer.h>
#include <VulkanWrapper/Image/ImageLoader.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Memory/StagingBufferManager.h>
#include <VulkanWrapper/Model/Importer.h>
#include <VulkanWrapper/Model/Material/ColoredMaterialManager.h>
#include <VulkanWrapper/Model/Material/TexturedMaterialManager.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Pipeline/MeshRenderer.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
#include <VulkanWrapper/RenderPass/Attachment.h>
#include <VulkanWrapper/RenderPass/RenderPass.h>
#include <VulkanWrapper/RenderPass/Subpass.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/PhysicalDevice.h>
#include <VulkanWrapper/Vulkan/Queue.h>
#include <VulkanWrapper/Vulkan/Surface.h>
#include <VulkanWrapper/Vulkan/Swapchain.h>
#include <VulkanWrapper/Window/SDL_Initializer.h>
#include <VulkanWrapper/Window/Window.h>

std::vector<std::shared_ptr<const vw::ImageView>>
create_image_views(const vw::Device &device, const vw::Swapchain &swapchain) {
    std::vector<std::shared_ptr<const vw::ImageView>> result;
    for (auto &&image : swapchain.images()) {
        auto imageView = vw::ImageViewBuilder(device, image)
                             .setImageType(vk::ImageViewType::e2D)
                             .build();
        result.push_back(std::move(imageView));
    }
    return result;
}

struct UBOData {
    glm::mat4 proj = [] {
        auto proj = glm::perspective(glm::radians(45.0F), 1024.0F / 800.0F, 1.F,
                                     10000.0F);
        proj[1][1] *= -1;
        return proj;
    }();
    glm::mat4 view =
        glm::lookAt(glm::vec3(0.0F, 300.0F, 0.0F),
                    glm::vec3(1.0F, 300.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    glm::mat4 model = glm::mat4(1.0);
};

vw::Buffer<UBOData, true, vw::UniformBufferUsage>
createUbo(vw::Allocator &allocator) {
    auto buffer =
        allocator.create_buffer<UBOData, true, vw::UniformBufferUsage>(1);
    UBOData data;
    buffer.copy({&data, 1}, 0);
    return buffer;
}

std::vector<vw::Framebuffer> createFramebuffers(
    vw::Device &device, const vw::RenderPass &renderPass,
    const vw::Swapchain &swapchain,
    const std::vector<std::shared_ptr<const vw::ImageView>> &images,
    const std::shared_ptr<const vw::ImageView> &depth_buffer) {
    std::vector<vw::Framebuffer> framebuffers;

    for (const auto &imageView : images) {
        auto framebuffer =
            vw::FramebufferBuilder(device, renderPass, swapchain.width(),
                                   swapchain.height())
                .add_attachment(imageView)
                .add_attachment(depth_buffer)
                .build();
        framebuffers.push_back(std::move(framebuffer));
    }

    return framebuffers;
}

vw::Pipeline create_pipeline(
    const vw::Device &device, const vw::RenderPass &render_pass,
    std::shared_ptr<const vw::ShaderModule> vertex,
    std::shared_ptr<const vw::ShaderModule> fragment,
    std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
    std::shared_ptr<const vw::DescriptorSetLayout> material_layout,
    vw::Width width, vw::Height height) {

    auto pipelineLayout = vw::PipelineLayoutBuilder(device)
                              .with_descriptor_set_layout(uniform_buffer_layout)
                              .with_descriptor_set_layout(material_layout)
                              .build();

    return vw::GraphicsPipelineBuilder(device, render_pass, 1,
                                       std::move(pipelineLayout))
        .add_vertex_binding<vw::FullVertex3D>()
        .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex))
        .add_shader(vk::ShaderStageFlagBits::eFragment, std::move(fragment))
        .with_fixed_scissor(int32_t(width), int32_t(height))
        .with_fixed_viewport(int32_t(width), int32_t(height))
        .with_depth_test(false, vk::CompareOp::eEqual)
        .add_color_attachment()
        .build();
}

vw::Pipeline create_zpass_pipeline(
    const vw::Device &device, const vw::RenderPass &render_pass,
    std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
    vw::Width width, vw::Height height) {
    auto vertex_shader = vw::ShaderModule::create_from_spirv_file(
        device, "../../../examples/Advanced/Shaders/bin/GBuffer/zpass.spv");

    auto pipeline_layout =
        vw::PipelineLayoutBuilder(device)
            .with_descriptor_set_layout(uniform_buffer_layout)
            .build();

    return vw::GraphicsPipelineBuilder(device, render_pass, 0,
                                       std::move(pipeline_layout))
        .add_vertex_binding<vw::Vertex3D>()
        .add_shader(vk::ShaderStageFlagBits::eVertex, std::move(vertex_shader))
        .with_fixed_scissor(int32_t(width), int32_t(height))
        .with_fixed_viewport(int32_t(width), int32_t(height))
        .with_depth_test(true, vk::CompareOp::eLess)
        .build();
}

vw::MeshRenderer create_renderer(
    const vw::Device &device, const vw::RenderPass &render_pass,
    const vw::Model::MeshManager &mesh_manager,
    const std::shared_ptr<const vw::DescriptorSetLayout> &uniform_buffer_layout,
    vw::Width width, vw::Height height) {
    auto vertexShader = vw::ShaderModule::create_from_spirv_file(
        device, "../../../examples/Advanced/Shaders/bin/GBuffer/"
                "gbuffer.spv");
    auto fragment_textured = vw::ShaderModule::create_from_spirv_file(
        device, "../../../examples/Advanced/Shaders/bin/GBuffer/"
                "gbuffer_textured.spv");
    auto fragment_colored = vw::ShaderModule::create_from_spirv_file(
        device, "../../../examples/Advanced/Shaders/bin/GBuffer/"
                "gbuffer_colored.spv");
    auto textured_pipeline =
        create_pipeline(device, render_pass, vertexShader, fragment_textured,
                        uniform_buffer_layout,
                        mesh_manager.material_manager_map().layout(
                            vw::Model::Material::textured_material_tag),
                        width, height);

    auto colored_pipeline =
        create_pipeline(device, render_pass, vertexShader, fragment_colored,
                        uniform_buffer_layout,
                        mesh_manager.material_manager_map().layout(
                            vw::Model::Material::colored_material_tag),
                        width, height);

    vw::MeshRenderer renderer;
    renderer.add_pipeline(vw::Model::Material::textured_material_tag,
                          std::move(textured_pipeline));
    renderer.add_pipeline(vw::Model::Material::colored_material_tag,
                          std::move(colored_pipeline));
    return renderer;
}

struct ZPassTag {};
struct ColorPassTag {};
const auto color_pass_tag = vw::create_subpass_tag<ColorPassTag>();
const auto z_pass_tag = vw::create_subpass_tag<ZPassTag>();

class ZPass : public vw::Subpass {
  public:
    ZPass(const vw::Device &device, const vw::Model::MeshManager &mesh_manager,
          std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
          vw::Width width, vw::Height height, vk::DescriptorSet descriptor_set)
        : m_device{device}
        , m_mesh_manager{mesh_manager}
        , m_uniform_buffer_layout{uniform_buffer_layout}
        , m_width{width}
        , m_height{height}
        , m_descriptor_set{descriptor_set} {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        const auto &meshes = m_mesh_manager.meshes();
        std::span first_descriptor_sets = {&m_descriptor_set, 1};
        cmd_buffer.bindPipeline(pipeline_bind_point(), m_pipeline->handle());
        cmd_buffer.bindDescriptorSets(pipeline_bind_point(),
                                      m_pipeline->layout().handle(), 0,
                                      first_descriptor_sets, nullptr);
        for (const auto &mesh : meshes) {
            mesh.draw_zpass(cmd_buffer);
        }
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        return m_color_attachments;
    }

    const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept override {
        return &m_depth_stencil_attachment;
    }

    vw::SubpassDependencyMask input_dependencies() const noexcept override {
        return {};
    }

    vw::SubpassDependencyMask output_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        mask.stage = vk::PipelineStageFlagBits::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits::eLateFragmentTests;
        return mask;
    }

  protected:
    void initialize(const vw::RenderPass &render_pass) override {
        m_pipeline = create_zpass_pipeline(
            m_device, render_pass, m_uniform_buffer_layout, m_width, m_height);
    }

  private:
    const vw::Device &m_device;
    const vw::Model::MeshManager &m_mesh_manager;
    std::shared_ptr<const vw::DescriptorSetLayout> m_uniform_buffer_layout;
    vw::Width m_width;
    vw::Height m_height;
    vk::DescriptorSet m_descriptor_set;
    std::optional<vw::Pipeline> m_pipeline;

    inline static const vk::AttachmentReference2 m_depth_stencil_attachment =
        vk::AttachmentReference2(
            1, vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageAspectFlagBits::eDepth);

    inline static const std::vector<vk::AttachmentReference2>
        m_color_attachments = {};
};

class ColorSubpass : public vw::Subpass {
  public:
    ColorSubpass(
        const vw::Device &device, const vw::Model::MeshManager &mesh_manager,
        std::shared_ptr<const vw::DescriptorSetLayout> uniform_buffer_layout,
        vw::Width width, vw::Height height, vk::DescriptorSet descriptor_set)
        : m_device{device}
        , m_mesh_manager{mesh_manager}
        , m_uniform_buffer_layout{uniform_buffer_layout}
        , m_width{width}
        , m_height{height}
        , m_descriptor_set{descriptor_set} {}

    void execute(vk::CommandBuffer cmd_buffer) const noexcept override {
        const auto &meshes = m_mesh_manager.meshes();
        std::span first_descriptor_sets = {&m_descriptor_set, 1};
        for (const auto &mesh : meshes) {
            m_mesh_renderer.draw_mesh(cmd_buffer, mesh, first_descriptor_sets);
        }
    }

    const std::vector<vk::AttachmentReference2> &
    color_attachments() const noexcept override {
        return m_color_attachments;
    }

    const vk::AttachmentReference2 *
    depth_stencil_attachment() const noexcept override {
        return &m_depth_stencil_attachment;
    }

    vw::SubpassDependencyMask input_dependencies() const noexcept override {
        vw::SubpassDependencyMask mask;
        mask.access = vk::AccessFlagBits::eDepthStencilAttachmentRead;
        mask.stage = vk::PipelineStageFlagBits::eEarlyFragmentTests |
                     vk::PipelineStageFlagBits::eLateFragmentTests;
        return mask;
    }

    vw::SubpassDependencyMask output_dependencies() const noexcept override {
        return {};
    }

  protected:
    void initialize(const vw::RenderPass &render_pass) override {
        m_mesh_renderer =
            create_renderer(m_device, render_pass, m_mesh_manager,
                            m_uniform_buffer_layout, m_width, m_height);
    }

  private:
    const vw::Device &m_device;
    const vw::Model::MeshManager &m_mesh_manager;
    std::shared_ptr<const vw::DescriptorSetLayout> m_uniform_buffer_layout;
    vw::Width m_width;
    vw::Height m_height;
    vw::MeshRenderer m_mesh_renderer;
    vk::DescriptorSet m_descriptor_set;

    inline static const vk::AttachmentReference2 m_depth_stencil_attachment =
        vk::AttachmentReference2(1,
                                 vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                                 vk::ImageAspectFlagBits::eDepth);

    inline static const std::vector<vk::AttachmentReference2>
        m_color_attachments = {vk::AttachmentReference2(
            0, vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageAspectFlagBits::eColor)};
};

int main() {
    try {
        vw::SDL_Initializer initializer;
        vw::Window window = vw::WindowBuilder(initializer)
                                .with_title("Coucou")
                                .sized(vw::Width(1024), vw::Height(800))
                                .build();

        vw::Instance instance =
            vw::InstanceBuilder()
                .addPortability()
                .addExtensions(window.get_required_instance_extensions())
                .setApiVersion(vw::ApiVersion::e13)
                .build();

        auto surface = window.create_surface(instance);

        auto device = instance.findGpu()
                          .with_queue(vk::QueueFlagBits::eGraphics |
                                      vk::QueueFlagBits::eCompute |
                                      vk::QueueFlagBits::eTransfer)
                          .with_presentation(surface.handle())
                          .with_synchronization_2()
                          .build();

        auto allocator = vw::AllocatorBuilder(instance, device).build();

        auto swapchain = window.create_swapchain(device, surface.handle());

        auto descriptor_set_layout =
            vw::DescriptorSetLayoutBuilder(device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                .build();

        auto uniform_buffer = createUbo(allocator);

        auto descriptor_pool =
            vw::DescriptorPoolBuilder(device, descriptor_set_layout).build();

        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(0, uniform_buffer.handle(), 0,
                                                uniform_buffer.size_bytes());

        auto descriptor_set =
            descriptor_pool.allocate_set(descriptor_allocator);

        vw::Model::MeshManager mesh_manager(device, allocator);
        mesh_manager.read_file("../../../Models/Sponza/sponza.obj");
        mesh_manager.read_file("../../../Models/cube.obj");

        const auto depth_buffer = allocator.create_image_2D(
            swapchain.width(), swapchain.height(), false,
            vk::Format::eD24UnormS8Uint,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);

        const auto depth_buffer_view =
            vw::ImageViewBuilder(device, depth_buffer)
                .setImageType(vk::ImageViewType::e2D)
                .build();

        const auto color_attachment =
            vw::AttachmentBuilder{}
                .with_format(swapchain.format())
                .with_final_layout(vk::ImageLayout::ePresentSrcKHR)
                .build();

        const auto depth_attachment =
            vw::AttachmentBuilder{}
                .with_format(depth_buffer->format())
                .with_final_layout(
                    vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .build();

        auto depth_subpass = std::make_unique<ZPass>(
            device, mesh_manager, descriptor_set_layout, swapchain.width(),
            swapchain.height(), descriptor_set);
        auto color_subpass = std::make_unique<ColorSubpass>(
            device, mesh_manager, descriptor_set_layout, swapchain.width(),
            swapchain.height(), descriptor_set);

        auto renderPass =
            vw::RenderPassBuilder(device)
                .add_attachment(color_attachment, vk::ClearColorValue())
                .add_attachment(depth_attachment,
                                vk::ClearDepthStencilValue(1.0))
                .add_subpass(z_pass_tag, std::move(depth_subpass))
                .add_subpass(color_pass_tag, std::move(color_subpass))
                .add_dependency(z_pass_tag, color_pass_tag)
                .build();

        auto commandPool = vw::CommandPoolBuilder(device).build();
        auto image_views = create_image_views(device, swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        const auto framebuffers = createFramebuffers(
            device, renderPass, swapchain, image_views, depth_buffer_view);

        const vk::Extent2D extent(uint32_t(swapchain.width()),
                                  uint32_t(swapchain.height()));

        for (auto [framebuffer, commandBuffer] :
             std::views::zip(framebuffers, commandBuffers)) {
            vw::CommandBufferRecorder recorder(commandBuffer);
            renderPass.execute(commandBuffer, framebuffer,
                               std::span{&descriptor_set, 1});
        }

        auto renderFinishedSemaphore = vw::SemaphoreBuilder(device).build();
        auto imageAvailableSemaphore = vw::SemaphoreBuilder(device).build();

        auto cmd_buffer = mesh_manager.fill_command_buffer();

        device.graphicsQueue().enqueue_command_buffer(cmd_buffer);

        while (!window.is_close_requested()) {
            window.update();

            auto index = swapchain.acquire_next_image(imageAvailableSemaphore);

            const vk::PipelineStageFlags waitStage =
                vk::PipelineStageFlagBits::eTopOfPipe;

            const auto image_available_handle =
                imageAvailableSemaphore.handle();
            const auto render_finished_handle =
                renderFinishedSemaphore.handle();

            device.graphicsQueue().enqueue_command_buffer(
                commandBuffers[index]);

            auto fence = device.graphicsQueue().submit(
                {&waitStage, 1}, {&image_available_handle, 1},
                {&render_finished_handle, 1});

            device.presentQueue().present(swapchain, index,
                                          renderFinishedSemaphore);
        }

        device.wait_idle();
    } catch (const vw::Exception &exception) {
        std::cout << exception.m_sourceLocation.function_name() << '\n';
    }
}
