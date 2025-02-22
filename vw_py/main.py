from Window import SDL_Initializer, Window
from Vulkan import Device, Instance
from Pipeline import ShaderModule, PipelineLayout, Pipeline
from RenderPass import Attachment, Subpass, RenderPass
from Command import CommandPool
import bindings_vw_py as bindings

def create_image_views(device, swapchain):
    result = []
    for image in swapchain.images():
        image_view = ImageView.ImageViewBuilder(device, image).\
            set_image_type(e2D).\
            build()
        result.append(image_view)
    return result

class App:
    resource = []

    def __setattr__(self, name, value):
        super().__setattr__(name, value)
        if value is not None:
            self.resource.append((name, value))

    def __enter__(self):
        return self
    
    def __exit__(self, a, b, c):
        while self.resource:
            (name, elem) = self.resource.pop()
            self.__setattr__(name, None)
            del elem

with App() as app:
    app.initializer = SDL_Initializer.SDL_Initializer()

    app.window = Window.WindowBuilder(app.initializer)\
        .sized(800, 600)\
        .with_title("From Python")\
        .build()

    app.instance = Instance.InstanceBuilder().\
        add_extensions(app.window.get_required_extensions()).\
        set_debug_mode().\
        build()

    app.surface = app.window.create_surface(app.instance)

    app.device = app.instance.find_gpu().\
        with_presentation(app.surface).\
        with_queue(bindings.VwQueueFlagBits_Graphics | \
                   bindings.VwQueueFlagBits_Compute | \
                   bindings.VwQueueFlagBits_Transfer).\
        build()

    app.swapchain = app.window.create_swapchain(app.device, app.surface)

    app.vertex_shader = ShaderModule.from_spirv_file(app.device, "../Shaders/bin/vert.spv")
    app.fragment_shader = ShaderModule.from_spirv_file(app.device, "../Shaders/bin/frag.spv")

    app.pipeline_layout = PipelineLayout.PipelineLayoutBuilder(app.device).build()

    app.attachment = Attachment.AttachmentBuilder("COLOR").\
        with_format(app.swapchain.format()).\
        with_final_layout(bindings.VwImageLayout_PresentSrcKHR).\
        build()

    app.subpass = Subpass.SubpassBuilder().\
        add_color_attachment(app.attachment, bindings.VwImageLayout_AttachmentOptimal).\
        build()

    app.render_pass = RenderPass.RenderPassBuilder(app.device).\
        add_subpass(app.subpass).\
        build()

    app.pipeline = Pipeline.GraphicsPipelineBuilder(app.device, app.render_pass).\
        add_shader(bindings.VwShaderStageFlagBits_Vertex, app.vertex_shader).\
        add_shader(bindings.VwShaderStageFlagBits_Fragment, app.fragment_shader).\
        with_fixed_scissor(app.swapchain.width(), app.swapchain.height()).\
        with_fixed_viewport(app.swapchain.width(), app.swapchain.height()).\
        with_pipeline_layout(app.pipeline_layout).\
        add_color_attachment().\
        build()

    app.command_pool = CommandPool.CommandPoolBuilder(app.device).build()
    app.command_buffers = app.command_pool.allocate(3)

    print(app.command_buffers)

    while not app.window.is_close_requested():
        app.window.update()
