use std::{env, fs};
use tempfile::TempDir;

mod configpaths_helpers;

#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_empty_Newsboat_dotdir_already_exists(
) {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    configpaths_helpers::mock_newsbeuter_dotdir(&tmp);

    let dotdir = tmp.path().join(".Newsboat");
    assert!(fs::create_dir(&dotdir).is_ok());

    configpaths_helpers::assert_dotdir_not_migrated(&dotdir);
}
