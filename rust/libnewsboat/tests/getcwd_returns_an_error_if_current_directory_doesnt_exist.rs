use libnewsboat::utils;
use serial_test::serial;
use std::env;
use tempfile::TempDir;

#[test]
#[serial] // Because it changes the current working directory
fn t_getcwd_returns_an_error_if_current_directory_doesnt_exist() {
    {
        let tmp = TempDir::new().unwrap();
        let chdir_result = env::set_current_dir(tmp.path());
        assert!(chdir_result.is_ok());
    }

    // `tmp` went out of scope and removed the directory, but we're still in it

    assert!(utils::getcwd().is_err());
}
