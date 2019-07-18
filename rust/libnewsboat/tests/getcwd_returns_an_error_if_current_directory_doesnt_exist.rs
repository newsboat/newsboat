extern crate libnewsboat;
extern crate tempfile;

use self::tempfile::TempDir;
use libnewsboat::utils;
use std::env;

#[test]
fn t_getcwd_returns_an_error_if_current_directory_doesnt_exist() {
    {
        let tmp = TempDir::new().unwrap();
        let chdir_result = env::set_current_dir(&tmp.path());
        assert!(chdir_result.is_ok());
    }

    // `tmp` went out of scope and removed the directory, but we're still in it

    assert!(utils::getcwd().is_err());
}
