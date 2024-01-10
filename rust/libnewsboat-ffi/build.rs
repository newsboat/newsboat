fn add_cxxbridge(module: &str) {
    cxx_build::bridge(format!("src/{module}.rs"))
        .flag("-std=c++11")
        // Disable warnings in generated code, since we can't affect them directly. We have to do
        // this because these warnings are turned into errors when we pass `-D warnings` to Cargo
        // on CI.
        .flag("-w")
        .compile(&format!("libnewsboat-ffi-{module}"));
    println!("cargo:rerun-if-changed=src/{module}.rs");
}

fn main() {
    add_cxxbridge("charencoding");
    add_cxxbridge("cliargsparser");
    add_cxxbridge("configpaths");
    add_cxxbridge("fmtstrformatter");
    add_cxxbridge("fslock");
    add_cxxbridge("history");
    add_cxxbridge("keymap");
    add_cxxbridge("logger");
    add_cxxbridge("scopemeasure");
    add_cxxbridge("utils");
}
