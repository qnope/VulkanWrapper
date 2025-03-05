use std::env;
use std::fs::copy;
use std::path::PathBuf;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();

    let core_library = String::from("libVulkanWrapperCoreLibrary.dylib");
    let wrapper_library = String::from("libVulkanWrapperWrapperLibrary.dylib");

    let deps_core = format!("{}/../../../deps/{}", out_dir, core_library);
    let deps_wrapper = format!("{}/../../../deps/{}", out_dir, wrapper_library);

    println!(
        "cargo:rustc-link-search=native={}",
        env::var("DYLD_LIBRARY_PATH").unwrap()
    );
    println!("cargo:rustc-link-search=native={}", ".");
    println!("cargo:rustc-link-lib=dylib=VulkanWrapperWrapperLibrary");
    println!("cargo:rustc-link-lib=dylib=vulkan");
    println!("cargo::rerun-if-changed={}", core_library);
    println!("cargo::rerun-if-changed={}", wrapper_library);

    copy(core_library.clone(), deps_core).unwrap();
    copy(wrapper_library.clone(), deps_wrapper).unwrap();

    let bindings = bindgen::Builder::default()
        .header("../vw_c/include/bindings.h")
        .clang_args(["-x", "c++"])
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .default_enum_style(bindgen::EnumVariation::Rust {
            non_exhaustive: false,
        })
        .bitfield_enum(".*Bits")
        .generate()
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(out_dir);
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
