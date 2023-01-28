use std::process::Command;

fn main() {
    // Code lifted from https://stackoverflow.com/a/44407625/2350060
    let command_output = Command::new("git")
        .args(["describe", "--abbrev=4", "--dirty", "--always", "--tags"])
        .output();
    match command_output {
        Ok(ref hash_output) if hash_output.status.success() => {
            let hash = String::from_utf8_lossy(&hash_output.stdout);
            println!("cargo:rustc-env=NEWSBOAT_VERSION={hash}");
            // Re-build this crate when Git HEAD changes. Idea lifted from vergen crate.
            println!("cargo:rebuild-if-changed=.git/HEAD");
        }

        _ => println!(
            "cargo:rustc-env=NEWSBOAT_VERSION={}.{}.{}",
            env!("CARGO_PKG_VERSION_MAJOR"),
            env!("CARGO_PKG_VERSION_MINOR"),
            env!("CARGO_PKG_VERSION_PATCH")
        ),
    }
}
