fn add_cxxbridge(module: &str) {
    cxx_build::bridge(format!("src/{}.rs", module))
        .flag("-std=c++11")
        .compile(&format!("libnewsboat-ffi-{}", module));
    println!("cargo:rerun-if-changed=src/{}.rs", module);
}

fn main() {
    add_cxxbridge("fslock");
    add_cxxbridge("keymap");
    add_cxxbridge("scopemeasure");
    add_cxxbridge("utils");
}
