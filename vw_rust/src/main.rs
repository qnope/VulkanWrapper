use vulkan_wrapper::command::command_pool::CommandPoolBuilder;
use vulkan_wrapper::pipeline::pipeline::GraphicsPipelineBuilder;
use vulkan_wrapper::pipeline::pipeline_layout::PipelineLayoutBuilder;
use vulkan_wrapper::pipeline::shaders::*;
use vulkan_wrapper::render_pass::attachment::*;
use vulkan_wrapper::render_pass::render_pass::RenderPassBuilder;
use vulkan_wrapper::render_pass::subpass::SubpassBuilder;
use vulkan_wrapper::synchronization::fence::FenceBuilder;
use vulkan_wrapper::synchronization::semaphore::SemaphoreBuilder;
use vulkan_wrapper::sys::bindings::{VkImageLayout, VkQueueFlagBits, VkShaderStageFlagBits};
use vulkan_wrapper::vulkan::instance::*;
use vulkan_wrapper::window::sdl_initializer::*;
use vulkan_wrapper::window::window::*;

fn main() {
    let initializer = SdlInitializer::new();

    let window = WindowBuilder::new(&initializer)
        .sized(800, 600)
        .with_title("Test from Rust")
        .build();

    let extensions = window.get_required_extensions();

    let instance = InstanceBuilder::new().add_extensions(extensions).build();
    let surface = window.create_surface(&instance);

    let device = instance
        .find_gpu()
        .with_queue(
            VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT
                | VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT
                | VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT,
        )
        .with_presentation(&surface)
        .build();

    let swapchain = window.create_swapchain(&device, &surface);

    let vertex_shader = ShaderModule::create_from_spirv_file(&device, "../Shaders/bin/vert.spv");

    let fragment_shader = ShaderModule::create_from_spirv_file(&device, "../Shaders/bin/frag.spv");

    let pipeline_layout = PipelineLayoutBuilder::new(&device).build();

    let attachment = AttachmentBuilder::new("COLOR")
        .with_format(swapchain.format())
        .with_final_layout(VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        .build();

    let subpass = SubpassBuilder::new()
        .add_color_attachment(
            attachment,
            VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        )
        .build();

    let render_pass = RenderPassBuilder::new(&device).add_subpass(subpass).build();

    let _pipeline = GraphicsPipelineBuilder::new(&device, &render_pass)
        .add_shader(
            VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
            vertex_shader,
        )
        .add_shader(
            VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
            fragment_shader,
        )
        .with_fixed_viewport((swapchain.width(), swapchain.height()))
        .with_fixed_scissor((swapchain.width(), swapchain.height()))
        .with_pipeline_layout(&pipeline_layout)
        .add_color_attachment()
        .build();

    let _fence = FenceBuilder::new(&device).build();

    let _render_finished_semaphore = SemaphoreBuilder::new(&device).build();
    let _image_available_semaphore = SemaphoreBuilder::new(&device).build();

    let _command_pool = CommandPoolBuilder::new(&device).build();

    let _images = swapchain.images();

    while !window.is_close_requested() {
        window.update();
    }
}
