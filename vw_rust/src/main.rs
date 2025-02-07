use vulkan_wrapper::window::sdl_initializer::*;
use vulkan_wrapper::window::window::*;

fn main() {
    // calling the function from foo library
    let initializer = SdlInitializer::new();

    let mut window = WindowBuilder::new(&initializer)
        .sized(800, 600)
        .with_title(String::from("Test from Rust"))
        .build();

    let extensions = window.get_required_extensions();

    for elem in extensions {
        println!("{elem}");
    }

    while !window.is_close_requested() {
        window.update();
    }
}
