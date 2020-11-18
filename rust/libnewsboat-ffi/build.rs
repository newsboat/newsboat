fn main() {
    cxx_build::bridge("src/utils.rs")
        .flag("-std=c++11")
        .compile("libnewsboat-ffi-utils");
    println!("cargo:rerun-if-changed=src/utils.rs");
}
