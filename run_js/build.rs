use std::env::var;

fn main() {
    let manifest_dir = var("CARGO_MANIFEST_DIR").unwrap();
    println!("cargo:rustc-link-search={}/../bendermate/target/debug", manifest_dir);
    println!("cargo:rustc-link-lib=dylib=bendermate");
    println!("cargo:rustc-link-search={}/../bendermate", manifest_dir);
    println!("cargo:rustc-link-lib=bmonmate");
}
