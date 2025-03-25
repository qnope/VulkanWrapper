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
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Pipeline/PipelineLayout.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>
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

constexpr std::string_view COLOR = "COLOR";
constexpr std::string_view DEPTH = "DEPTH";

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
    glm::mat4 view = glm::lookAt(glm::vec3(-30.0F, 300.0F, 0.0F),
                                 glm::vec3(10.0F, 300.0F, 0.0F),
                                 glm::vec3(0.0F, 1.0F, 0.0F));
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

void record(vk::CommandBuffer commandBuffer, vk::Extent2D extent,
            const vw::Framebuffer &framebuffer, const vw::Pipeline &pipeline,
            const vw::RenderPass &renderPass,
            const std::vector<vw::Model::Mesh> &meshes,
            const vw::PipelineLayout &layout, vk::DescriptorSet ubo_set) {
    auto recorder = vw::CommandBufferRecorder(commandBuffer);
    auto rp = recorder.begin_render_pass(renderPass, framebuffer);
    auto pi = rp.bind_graphics_pipeline(pipeline);
    pi.bind_descriptor_set(layout, 0, std::span(&ubo_set, 1), {});
    for (const auto &m : meshes) {
        m.draw(pi.m_commandBuffer, layout);
    }
}

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

        auto vertexShader = vw::ShaderModule::create_from_spirv_file(
            device, "../../Shaders/bin/vert.spv");

        auto fragmentShader = vw::ShaderModule::create_from_spirv_file(
            device, "../../Shaders/bin/frag.spv");

        auto descriptor_set_layout =
            vw::DescriptorSetLayoutBuilder(device)
                .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
                .build();

        vw::Model::MeshManager mesh_manager(device, allocator);
        mesh_manager.read_file("../../Models/Sponza/sponza.obj");

        auto pipelineLayout =
            vw::PipelineLayoutBuilder(device)
                .with_descriptor_set_layout(descriptor_set_layout)
                .with_descriptor_set_layout(mesh_manager.layout())
                .build();

        const auto depth_buffer = allocator.create_image_2D(
            swapchain.width(), swapchain.height(), false,
            vk::Format::eD24UnormS8Uint,
            vk::ImageUsageFlagBits::eDepthStencilAttachment);

        const auto depth_buffer_view =
            vw::ImageViewBuilder(device, depth_buffer)
                .setImageType(vk::ImageViewType::e2D)
                .build();

        const auto color_attachment =
            vw::AttachmentBuilder(COLOR)
                .with_format(swapchain.format(), vk::ClearColorValue())
                .with_final_layout(vk::ImageLayout::ePresentSrcKHR)
                .build();

        const auto depth_attachment =
            vw::AttachmentBuilder(DEPTH)
                .with_format(depth_buffer->format(),
                             vk::ClearDepthStencilValue(1.0))
                .with_final_layout(
                    vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .build();

        auto subpass =
            vw::SubpassBuilder()
                .add_color_attachment(color_attachment,
                                      vk::ImageLayout::eAttachmentOptimal)
                .add_depth_stencil_attachment(depth_attachment)
                .build();

        auto renderPass = vw::RenderPassBuilder(device)
                              .add_subpass(std::move(subpass))
                              .build();

        auto pipeline = vw::GraphicsPipelineBuilder(device, renderPass)
                            .add_vertex_binding<vw::FullVertex3D>()
                            .add_shader(vk::ShaderStageFlagBits::eVertex,
                                        std::move(vertexShader))
                            .add_shader(vk::ShaderStageFlagBits::eFragment,
                                        std::move(fragmentShader))
                            .with_fixed_scissor(int32_t(swapchain.width()),
                                                int32_t(swapchain.height()))
                            .with_fixed_viewport(int32_t(swapchain.width()),
                                                 int32_t(swapchain.height()))
                            .with_depth_test(true, vk::CompareOp::eLess)
                            .with_pipeline_layout(pipelineLayout)
                            .add_color_attachment()
                            .build();

        auto commandPool = vw::CommandPoolBuilder(device).build();
        auto image_views = create_image_views(device, swapchain);
        auto commandBuffers = commandPool.allocate(image_views.size());

        const auto framebuffers = createFramebuffers(
            device, renderPass, swapchain, image_views, depth_buffer_view);

        const vk::Extent2D extent(uint32_t(swapchain.width()),
                                  uint32_t(swapchain.height()));

        auto uniform_buffer = createUbo(allocator);

        auto descriptor_pool =
            vw::DescriptorPoolBuilder(device, descriptor_set_layout).build();

        vw::DescriptorAllocator descriptor_allocator;
        descriptor_allocator.add_uniform_buffer(0, uniform_buffer.handle(), 0,
                                                uniform_buffer.size_bytes());

        auto descriptor_set =
            descriptor_pool.allocate_set(descriptor_allocator);

        for (auto [framebuffer, commandBuffer] :
             std::views::zip(framebuffers, commandBuffers)) {
            record(commandBuffer, extent, framebuffer, pipeline, renderPass,
                   mesh_manager.meshes(), pipelineLayout, descriptor_set);
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
