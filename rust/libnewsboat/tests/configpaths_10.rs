use section_testing::{enable_sections, section};
use std::{env, fs};
use tempfile::TempDir;

mod configpaths_helpers;

enable_sections! {
#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_empty_newsboat_xdg_config_dir_already_exists(
) {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    if section!("Default XDG locations") {
        configpaths_helpers::mock_newsbeuter_xdg_dirs(&tmp);

        let config_dir = tmp.path().join(".config").join("newsboat");
        assert!(fs::create_dir(&config_dir).is_ok());

        let data_dir = tmp.path().join(".local").join("share").join("newsboat");
        configpaths_helpers::assert_xdg_not_migrated(&config_dir, &data_dir);
    }

    if section!("XDG_CONFIG_HOME redefined") {
        let config_home = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_home).is_ok());
        env::set_var("XDG_CONFIG_HOME", &config_home);

        configpaths_helpers::mock_xdg_dirs(
            &config_home.join("newsbeuter"),
            &tmp.path().join(".local").join("share").join("newsbeuter"));

        let config_dir = config_home.join("newsboat");
        assert!(fs::create_dir(&config_dir).is_ok());
        configpaths_helpers::assert_xdg_not_migrated(
            &config_dir,
            &tmp.path().join(".local").join("share").join("newsboat"));
    }

    if section!("XDG_DATA_HOME redefined") {
        let data_dir = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_dir).is_ok());
        env::set_var("XDG_DATA_HOME", &data_dir);

        configpaths_helpers::mock_xdg_dirs(
            &tmp.path().join(".config").join("newsbeuter"),
            &data_dir.join("newsbeuter"));

        let config_dir = tmp.path().join(".config").join("newsboat");
        assert!(fs::create_dir(&config_dir).is_ok());
        configpaths_helpers::assert_xdg_not_migrated(&config_dir, &data_dir.join("newsboat"));
    }

    if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
        let config_home = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_home).is_ok());
        env::set_var("XDG_CONFIG_HOME", &config_home);

        let data_dir = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_dir).is_ok());
        env::set_var("XDG_DATA_HOME", &data_dir);

        configpaths_helpers::mock_xdg_dirs(
            &config_home.join("newsbeuter"),
            &data_dir.join("newsbeuter"));

        let config_dir = config_home.join("newsboat");
        assert!(fs::create_dir(&config_dir).is_ok());
        configpaths_helpers::assert_xdg_not_migrated(&config_dir, &data_dir.join("newsboat"));
    }
}
}
