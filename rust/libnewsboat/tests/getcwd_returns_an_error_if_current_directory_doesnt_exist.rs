use libnewsboat::utils;
use std::env;
use tempfile::TempDir;

mod chdir_helpers;

#[test]
fn t_getcwd_returns_an_error_if_current_directory_doesnt_exist() {
    let tmp = TempDir::new().unwrap();
    let _chdir = chdir_helpers::Chdir::new(&tmp.path());

    drop(tmp);
    // `tmp` went out of scope and removed the directory, but we're still in it

    assert!(utils::getcwd().is_err());
}
