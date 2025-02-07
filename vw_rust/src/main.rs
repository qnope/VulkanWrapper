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
    let _surface = window.create_surface(&instance);

    while !window.is_close_requested() {
        window.update();
    }
}
