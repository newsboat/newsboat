extern crate libnewsboat;
extern crate tempfile;

use self::tempfile::TempDir;
use libnewsboat::utils;
use std::env;

#[test]
fn t_getcwd_value_depends_on_current_directory() {
    let initial_dir = utils::getcwd();
    assert!(initial_dir.is_ok());

    let tmp = TempDir::new().unwrap();
    assert_ne!(initial_dir.unwrap(), tmp.path());
    let chdir_result = env::set_current_dir(&tmp.path());
    assert!(chdir_result.is_ok());

    let new_dir = utils::getcwd();
    assert!(new_dir.is_ok());

    assert_eq!(new_dir.unwrap(), tmp.path());
}
