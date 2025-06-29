use std::env;
use tempfile::TempDir;

mod configpaths_helpers;

#[test]
fn t_configpaths_create_dirs_returns_false_if_dotdir_doeesnt_exist_and_couldnt_be_created() {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    unsafe { env::remove_var("XDG_CONFIG_HOME") };
    unsafe { env::remove_var("XDG_DATA_HOME") };

    configpaths_helpers::assert_create_dirs_returns_false(&tmp);
}
