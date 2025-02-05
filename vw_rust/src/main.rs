use vulkan_wrapper::window::window::*;
use vulkan_wrapper::window::sdl_initializer::*;

fn main() {
    // calling the function from foo library
    let initializer = SdlInitializer::new();
    let mut window = Window::new(&initializer);
    let extensions = window.get_required_extensions();

    for elem in extensions {
        println!("{elem}");
    }

    while !window.is_close_requested() {
        window.update();
    }
}