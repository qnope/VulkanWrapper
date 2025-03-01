from Window import SDL_Initializer, Window
from Vulkan import Device, Instance
from Pipeline import ShaderModule, PipelineLayout, Pipeline
from RenderPass import Attachment, Subpass, RenderPass
from Command import CommandPool, CommandBuffer
from Image import Image, ImageView, Framebuffer
from Synchronization import Fence, Semaphore
import bindings_vw_py as bindings

def create_image_views(device, swapchain):
    result = []
    for image in swapchain.images():
        image_view = ImageView.ImageViewBuilder(device, image).\
            with_type(bindings.VwImageViewType_Type2D).\
            build()
        result.append(image_view)
    return result

def create_framebuffers(device, render_pass, swapchain, image_views):
    result = []

    for image_view in image_views:
        framebuffer = Framebuffer.FramebufferBuilder(device, render_pass, swapchain.width(), swapchain.height()).\
            add_attachment(image_view).\
            build()
        result.append(framebuffer)
    return result

def record(cmd_buffer, framebuffer, pipeline, render_pass):
    with CommandBuffer.CommandBufferRecorder(cmd_buffer) as recorder:
        with recorder.begin_render_pass(render_pass, framebuffer) as render_pass_recorder:
            with render_pass_recorder.bind_graphics_pipeline(pipeline) as pipeline_recorder:
                pipeline_recorder.draw(3, 1, 0, 0)

initializer = SDL_Initializer.SDL_Initializer()

window = Window.WindowBuilder(initializer)\
    .sized(800, 600)\
    .with_title("From Python")\
    .build()

instance = Instance.InstanceBuilder().\
    add_extensions(window.get_required_extensions()).\
    set_debug_mode().\
    build()

surface = window.create_surface(instance)

device = instance.find_gpu().\
    with_presentation(surface).\
    with_queue(bindings.VwQueueFlagBits_Graphics | \
               bindings.VwQueueFlagBits_Compute | \
               bindings.VwQueueFlagBits_Transfer).\
    with_synchronization_2().\
    build()

swapchain = window.create_swapchain(device, surface)

vertex_shader = ShaderModule.from_spirv_file(device, "../Shaders/bin/vert.spv")
fragment_shader = ShaderModule.from_spirv_file(device, "../Shaders/bin/frag.spv")

pipeline_layout = PipelineLayout.PipelineLayoutBuilder(device).build()

attachment = Attachment.AttachmentBuilder("COLOR").\
    with_format(swapchain.format()).\
    with_final_layout(bindings.VwImageLayout_PresentSrcKHR).\
    build()

subpass = Subpass.SubpassBuilder().\
    add_color_attachment(attachment, bindings.VwImageLayout_AttachmentOptimal).\
    build()

render_pass = RenderPass.RenderPassBuilder(device).\
    add_subpass(subpass).\
    build()

pipeline = Pipeline.GraphicsPipelineBuilder(device, render_pass).\
    add_shader(bindings.VwShaderStageFlagBits_Vertex, vertex_shader).\
    add_shader(bindings.VwShaderStageFlagBits_Fragment, fragment_shader).\
    with_fixed_scissor(swapchain.width(), swapchain.height()).\
    with_fixed_viewport(swapchain.width(), swapchain.height()).\
    with_pipeline_layout(pipeline_layout).\
    add_color_attachment().\
    build()

command_pool = CommandPool.CommandPoolBuilder(device).build()
image_views = create_image_views(device, swapchain)
command_buffers = command_pool.allocate(len(image_views))

framebuffers = create_framebuffers(device, render_pass, swapchain, image_views)
    
fence = Fence.FenceBuilder(device).build()
render_finished_semaphore = Semaphore.SemaphoreBuilder(device).build()
image_available_semaphore = Semaphore.SemaphoreBuilder(device).build()
    
for (command_buffer, framebuffer) in zip(command_buffers, framebuffers):
    record(command_buffer, framebuffer, pipeline, render_pass)

while not window.is_close_requested():
    window.update()
    fence.wait()
    fence.reset()

    index = swapchain.acquire_next_image(image_available_semaphore)
    wait_stage = bindings.VwPipelineStageFlagBits_TopOfPipe

    device.graphics_queue().submit([command_buffers[index]], [wait_stage], [image_available_semaphore], [render_finished_semaphore], fence)
    device.present_queue().present(swapchain, index, render_finished_semaphore)
    
device.wait_idle()

print("Finished")