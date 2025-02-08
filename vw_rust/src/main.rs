use vulkan_wrapper::vulkan::device::QueueFlags;
use vulkan_wrapper::vulkan::instance::*;
use vulkan_wrapper::window::sdl_initializer::*;
use vulkan_wrapper::window::window::*;

fn main() {
    let initializer = SdlInitializer::new();

    let window = WindowBuilder::new(&initializer)
        .sized(800, 600)
        .with_title(String::from("Test from Rust"))
        .build();

    let extensions = window.get_required_extensions();

    let instance = InstanceBuilder::new().add_extensions(extensions).build();
    let surface = window.create_surface(&instance);

    let device = instance
        .find_gpu()
        .with_queue(QueueFlags::Graphics | QueueFlags::Compute | QueueFlags::Transfer)
        .with_presentation(&surface)
        .build();

    let _swapchain = window.create_swapchain(&device, &surface);

    while !window.is_close_requested() {
        window.update();
    }
}
