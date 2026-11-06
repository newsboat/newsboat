fn main() {
    cxx_build::bridges([
        "src/charencoding.rs",
        "src/filepath.rs",
        "src/cliargsparser.rs",
        "src/colormanager.rs",
        "src/configpaths.rs",
        "src/fmtstrformatter.rs",
        "src/fslock.rs",
        "src/history.rs",
        "src/keycombination.rs",
        "src/keymap.rs",
        "src/lineedit.rs",
        "src/logger.rs",
        "src/matchererror.rs",
        "src/scopemeasure.rs",
        "src/stflrichtext.rs",
        "src/utils.rs",
    ])
    // Keep in sync with c++ version specified in Makefile
    .std("c++17")
    // Disable warnings in generated code, since we can't affect them directly. We have to do
    // this because these warnings are turned into errors when we pass `-D warnings` to Cargo
    // on CI.
    .flag("-w")
    .compile("libnewsboat-ffi-cxx");
}
