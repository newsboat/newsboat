use section_testing::{enable_sections, section};
use std::{env, fs};
use tempfile::TempDir;

mod configpaths_helpers;

enable_sections! {
#[test]
fn t_configpaths_create_dirs_returns_false_if_xdg_config_dir_exists_but_data_dir_doesnt_and_couldnt_be_created(
) {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    unsafe { env::remove_var("XDG_CONFIG_HOME") };
    unsafe { env::remove_var("XDG_DATA_HOME") };

    if section!("Default XDG locations") {
        let config_dir = tmp.path().join(".config").join("newsboat");
        assert!(fs::create_dir_all(config_dir).is_ok());

        configpaths_helpers::assert_create_dirs_returns_false(&tmp);
    }

    if section!("XDG_CONFIG_HOME redefined") {
        let config_home = tmp.path().join("xdg-cfg");
        unsafe { env::set_var("XDG_CONFIG_HOME", &config_home) };

        let config_dir = config_home.join("newsboat");
        assert!(fs::create_dir_all(config_dir).is_ok());

        configpaths_helpers::assert_create_dirs_returns_false(&tmp);
    }

    if section!("XDG_DATA_HOME redefined") {
        let config_dir = tmp.path().join(".config").join("newsboat");
        assert!(fs::create_dir_all(config_dir).is_ok());

        let data_home = tmp.path().join("xdg-data");
        unsafe { env::set_var("XDG_DATA_HOME", data_home) };
        // It's important to set the variable, but *not* create the directory
        // - it's the pre-condition of the test that the data dir doesn't exist

        configpaths_helpers::assert_create_dirs_returns_false(&tmp);
    }

    if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
        let config_home = tmp.path().join("xdg-cfg");
        unsafe { env::set_var("XDG_CONFIG_HOME", &config_home) };

        let config_dir = config_home.join("newsboat");
        assert!(fs::create_dir_all(config_dir).is_ok());

        let data_home = tmp.path().join("xdg-data");
        unsafe { env::set_var("XDG_DATA_HOME", data_home) };
        // It's important to set the variable, but *not* create the directory
        // - it's the pre-condition of the test that the data dir doesn't exist

        configpaths_helpers::assert_create_dirs_returns_false(&tmp);
    }
}
}
