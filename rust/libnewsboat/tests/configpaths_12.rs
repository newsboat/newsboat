use std::env;
use tempfile::TempDir;

mod configpaths_helpers;
use crate::configpaths_helpers::libc::{S_IRUSR, S_IXUSR};

#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_newsboat_dotdir_couldnt_be_created(
) {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    unsafe { env::remove_var("XDG_CONFIG_HOME") };
    unsafe { env::remove_var("XDG_DATA_HOME") };

    configpaths_helpers::mock_newsbeuter_dotdir(&tmp);

    // Making home directory unwriteable makes it impossible to create
    // a directory there
    let _dotdir_chmod = configpaths_helpers::Chmod::new(tmp.path(), S_IRUSR | S_IXUSR);

    configpaths_helpers::assert_dotdir_not_migrated(&tmp.path().join(".newsboat"));
}
