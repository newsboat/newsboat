use std::process::Command;

fn main() {
    // Code lifted from https://stackoverflow.com/a/44407625/2350060
    // Panics in build script stop the build, which is fine by us - let's use unwrap() freely here.
    let hash_output = Command::new("git")
        .args(&["describe", "--abbrev=4", "--dirty", "--always", "--tags"])
        .output()
        .unwrap();
    let hash = String::from_utf8_lossy(&hash_output.stdout);
    println!("cargo:rustc-env=GIT_HASH={}", hash);
    // Re-build this crate when Git HEAD changes. Idea lifted from vergen crate.
    println!("cargo:rebuild-if-changed=.git/HEAD");
}
