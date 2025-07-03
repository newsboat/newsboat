use section_testing::{enable_sections, section};
use std::{env, fs};
use tempfile::TempDir;

mod configpaths_helpers;

enable_sections! {
#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_empty_newsboat_xdg_data_dir_already_exists(
) {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    unsafe { env::remove_var("XDG_CONFIG_HOME") };
    unsafe { env::remove_var("XDG_DATA_HOME") };

    if section!("Default XDG locations") {
        configpaths_helpers::mock_newsbeuter_xdg_dirs(&tmp);

        let config_dir = tmp.path().join(".config").join("newsboat");
        let data_dir = tmp.path().join(".local").join("share").join("newsboat");
        assert!(fs::create_dir(&data_dir).is_ok());

        configpaths_helpers::assert_xdg_not_migrated(&config_dir, &data_dir);
    }

    if section!("XDG_CONFIG_HOME redefined") {
        let config_home = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_home).is_ok());
        unsafe { env::set_var("XDG_CONFIG_HOME", &config_home) };

        configpaths_helpers::mock_xdg_dirs(
            &config_home.join("newsbeuter"),
            &tmp.path().join(".local").join("share").join("newsbeuter"));

        let data_dir = tmp.path().join(".local").join("share").join("newsboat");
        assert!(fs::create_dir(&data_dir).is_ok());
        configpaths_helpers::assert_xdg_not_migrated(
            &config_home.join("newsboat"),
            &data_dir);
    }

    if section!("XDG_DATA_HOME redefined") {
        let data_home = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_home).is_ok());
        unsafe { env::set_var("XDG_DATA_HOME", &data_home) };

        configpaths_helpers::mock_xdg_dirs(
            &tmp.path().join(".config").join("newsbeuter"),
            &data_home.join("newsbeuter"));

        let data_dir = data_home.join("newsboat");
        assert!(fs::create_dir(&data_dir).is_ok());
        configpaths_helpers::assert_xdg_not_migrated(&tmp.path().join(".config").join("newsboat"), &data_dir);
    }

    if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
        let config_home = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_home).is_ok());
        unsafe { env::set_var("XDG_CONFIG_HOME", &config_home) };

        let data_home = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_home).is_ok());
        unsafe { env::set_var("XDG_DATA_HOME", &data_home) };

        configpaths_helpers::mock_xdg_dirs(
            &config_home.join("newsbeuter"),
            &data_home.join("newsbeuter"));

        let data_dir = data_home.join("newsboat");
        assert!(fs::create_dir(&data_dir).is_ok());
        configpaths_helpers::assert_xdg_not_migrated(&config_home.join("newsboat"), &data_dir);
    }
}
}
