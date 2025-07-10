use libNewsboat::utils;
use std::env;
use tempfile::TempDir;

#[test]
fn t_getcwd_value_depends_on_current_directory() {
    let initial_dir = utils::getcwd().unwrap();

    let tmp = TempDir::new().unwrap();
    assert_ne!(&initial_dir, tmp.path());
    let chdir_result = env::set_current_dir(tmp.path());
    assert!(chdir_result.is_ok());

    let new_dir = utils::getcwd().unwrap();

    // We cannot assert that newdir == tmp.path() because the latter might contain symlinks, which
    // getcwd() resolves. So we have to test a slightly weaker property: changing the current
    // directory changes the return value of getcwd
    assert_ne!(initial_dir, new_dir);
}
