fn add_cxxbridge(module: &str) {
    cxx_build::bridge(format!("src/{module}.rs"))
        // Keep in sync with c++ version specified in Makefile
        .std("c++17")
        // Disable warnings in generated code, since we can't affect them directly. We have to do
        // this because these warnings are turned into errors when we pass `-D warnings` to Cargo
        // on CI.
        .flag("-w")
        .compile(&format!("libNewsboat-ffi-{module}"));
    println!("cargo:rerun-if-changed=src/{module}.rs");
}

fn main() {
    add_cxxbridge("charencoding");
    add_cxxbridge("cliargsparser");
    add_cxxbridge("configpaths");
    add_cxxbridge("fmtstrformatter");
    add_cxxbridge("fslock");
    add_cxxbridge("history");
    add_cxxbridge("keycombination");
    add_cxxbridge("keymap");
    add_cxxbridge("logger");
    add_cxxbridge("scopemeasure");
    add_cxxbridge("stflrichtext");
    add_cxxbridge("utils");
}
