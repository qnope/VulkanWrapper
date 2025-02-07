use std::env;
use std::path::PathBuf;

fn main() {
    let dst = cmake::Config::new("..").build();

    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=dylib=VulkanWrapper_cd");
    println!("cargo::rerun-if-changed=../vw_c");

    let bindings = bindgen::Builder::default()
        .header("../vw_c/include/bindings.h")
        .clang_args(["-x", "c++"])
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
