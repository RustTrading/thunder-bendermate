extern crate cmake;
use cmake::Config;

fn main() {
    let project_root_path = "../../..";
    let _dst = Config::new("bmonmate")
        .define("PROJECT_ROOT_PATH", project_root_path)
        .define("CMAKE_CURRENT_SOURCE_DIR", ".")
        .build();
    println!("cargo:rustc-link-search={}", ".");
    println!("cargo:rustc-link-lib=bmonmate");
    //println!("cargo:rustc-link-lib=benderjs");
}
