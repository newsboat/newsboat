use libNewsboat::utils;
use std::env;
use std::path::{Path, PathBuf};

#[test]
fn t_resolve_tilde() {
    env::set_var("HOME", "test");
    assert_eq!(&utils::resolve_tilde(PathBuf::from("~")), Path::new("test"));
    assert_eq!(
        &utils::resolve_tilde(PathBuf::from("~/")),
        Path::new("test/")
    );
    assert_eq!(
        &utils::resolve_tilde(PathBuf::from("~/dir")),
        Path::new("test/dir")
    );
    assert_eq!(
        &utils::resolve_tilde(PathBuf::from("/home/~")),
        Path::new("/home/~")
    );
    assert_eq!(
        &utils::resolve_tilde(PathBuf::from("~/foo/bar")),
        Path::new("test/foo/bar")
    );
    assert_eq!(
        &utils::resolve_tilde(PathBuf::from("/foo/bar")),
        Path::new("/foo/bar")
    );
}
