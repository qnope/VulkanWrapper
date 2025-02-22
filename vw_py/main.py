from Window import SDL_Initializer, Window
from Vulkan import Device, Instance
from Pipeline import ShaderModule
import bindings_vw_py as bindings

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
    build()

swapchain = window.create_swapchain(device, surface)

vertex_shader = ShaderModule.from_spirv_file(device, "../Shaders/bin/vert.spv")
fragment_shader = ShaderModule.from_spirv_file(device, "../Shaders/bin/frag.spv")

while not window.is_close_requested():
    window.update()

