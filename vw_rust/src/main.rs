use std::rc::Rc;
use vulkan_wrapper::command::command_buffer::CommandBuffer;
use vulkan_wrapper::command::command_buffer::CommandBufferRecorder;
use vulkan_wrapper::command::command_pool::CommandPoolBuilder;
use vulkan_wrapper::image::framebuffer::Framebuffer;
use vulkan_wrapper::image::framebuffer::FramebufferBuilder;
use vulkan_wrapper::image::image_view::ImageView;
use vulkan_wrapper::image::image_view::ImageViewBuilder;
use vulkan_wrapper::pipeline::pipeline::GraphicsPipelineBuilder;
use vulkan_wrapper::pipeline::pipeline::Pipeline;
use vulkan_wrapper::pipeline::pipeline_layout::PipelineLayoutBuilder;
use vulkan_wrapper::pipeline::shaders::*;
use vulkan_wrapper::render_pass::attachment::*;
use vulkan_wrapper::render_pass::render_pass::RenderPass;
use vulkan_wrapper::render_pass::render_pass::RenderPassBuilder;
use vulkan_wrapper::render_pass::subpass::SubpassBuilder;
use vulkan_wrapper::synchronization::fence::FenceBuilder;
use vulkan_wrapper::synchronization::semaphore::SemaphoreBuilder;
use vulkan_wrapper::sys::bindings::{VkImageLayout, VkQueueFlagBits, VkShaderStageFlagBits};
use vulkan_wrapper::vulkan::device::Device;
use vulkan_wrapper::vulkan::instance::*;
use vulkan_wrapper::vulkan::swapchain::Swapchain;
use vulkan_wrapper::window::sdl_initializer::*;
use vulkan_wrapper::window::window::*;
use std::iter::zip;

fn create_image_views<'a>(
    device: &'a Device<'a>,
    swapchain: &'a Swapchain<'a>,
) -> Vec<Rc<ImageView<'a>>> {
    let images = swapchain.images();
    images
        .iter()
        .map(|image| {
            let image_view = ImageViewBuilder::new(device, image.clone())
                .with_image_type(
                    vulkan_wrapper::sys::bindings::VkImageViewType::VK_IMAGE_VIEW_TYPE_2D,
                )
                .build();
            image_view
        })
        .collect()
}

fn create_frame_buffers<'a>(
    device: &'a Device,
    render_pass: &'a RenderPass<'a>,
    swapchain: &'a Swapchain<'a>,
    images: Vec<Rc<ImageView<'a>>>,
) -> Vec<Framebuffer<'a>> {
    images
        .into_iter()
        .map(|image_view| {
            FramebufferBuilder::new(
                device,
                render_pass,
                swapchain.width() as u32,
                swapchain.height() as u32,
            )
            .with_attachment(image_view)
            .build()
        })
        .collect()
}

fn record<'a>(
    cmd_buffer: &CommandBuffer<'a>,
    framebuffer: &Framebuffer<'a>,
    pipeline: &Pipeline<'a>,
    render_pass: &RenderPass<'a>,
) {
    CommandBufferRecorder::new(cmd_buffer)
        .begin_render_pass(render_pass, framebuffer)
        .bind_graphics_pipeline(pipeline)
        .draw(3, 1, 0, 0);
}

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

    let pipeline = GraphicsPipelineBuilder::new(&device, &render_pass)
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

    let command_pool = CommandPoolBuilder::new(&device).build();

    let image_views = create_image_views(&device, &swapchain);

    let framebuffers = create_frame_buffers(&device, &render_pass, &swapchain, image_views);

    let command_buffers = command_pool.allocate(framebuffers.len() as i32);

    for (framebuffer, cmd_buffer) in zip(framebuffers, command_buffers) {
        record(&cmd_buffer, &framebuffer, &pipeline, &render_pass);
    }

    while !window.is_close_requested() {
        window.update();
    }
}
