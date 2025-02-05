extern crate cmake;
use cmake::Config;

fn main()
{
    let dst = Config::new("..").build();       

    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=dylib=VulkanWrapper_cd");
    println!("cargo::rerun-if-changed=../vw_c"); 
}