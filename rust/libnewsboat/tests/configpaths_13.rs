use section_testing::{enable_sections, section};
use std::{env, fs};
use tempfile::TempDir;

mod configpaths_helpers;
use crate::configpaths_helpers::libc::{S_IRUSR, S_IXUSR};

enable_sections! {
#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_Newsboat_xdg_config_dir_couldnt_be_created(
) {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    if section!("Default XDG locations") {
        configpaths_helpers::mock_newsbeuter_xdg_dirs(&tmp);

        // Making XDG .config unwriteable makes it impossible to create
        // a directory there
        let config_home = tmp.path().join(".config");
        let _config_home_chmod = configpaths_helpers::Chmod::new(&config_home, S_IRUSR | S_IXUSR);

        let config_dir = config_home.join("Newsboat");
        let data_dir = tmp.path().join(".local").join("share").join("Newsboat");
        configpaths_helpers::assert_xdg_not_migrated(&config_dir, &data_dir);
    }

    if section!("XDG_CONFIG_HOME redefined") {
        let config_home = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_home).is_ok());
        env::set_var("XDG_CONFIG_HOME", &config_home);

        configpaths_helpers::mock_xdg_dirs(
            &config_home.join("newsbeuter"),
            &tmp.path().join(".local").join("share").join("newsbeuter"));

        // Making XDG .config unwriteable makes it impossible to create
        // a directory there
        let _config_home_chmod = configpaths_helpers::Chmod::new(&config_home, S_IRUSR | S_IXUSR);

        configpaths_helpers::assert_xdg_not_migrated(
            &config_home.join("Newsboat"),
            &tmp.path().join(".local").join("share").join("Newsboat"));
    }

    if section!("XDG_DATA_HOME redefined") {
        let data_dir = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_dir).is_ok());
        env::set_var("XDG_DATA_HOME", &data_dir);

        configpaths_helpers::mock_xdg_dirs(
            &tmp.path().join(".config").join("newsbeuter"),
            &data_dir.join("newsbeuter"));

        // Making XDG .config unwriteable makes it impossible to create
        // a directory there
        let config_home = tmp.path().join(".config");
        let _config_home_chmod = configpaths_helpers::Chmod::new(&config_home, S_IRUSR | S_IXUSR);

        configpaths_helpers::assert_xdg_not_migrated(
            &config_home.join("Newsboat"),
            &data_dir.join("Newsboat"));
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

        // Making XDG .config unwriteable makes it impossible to create
        // a directory there
        let _config_home_chmod = configpaths_helpers::Chmod::new(&config_home, S_IRUSR | S_IXUSR);

        configpaths_helpers::assert_xdg_not_migrated(
            &config_home.join("Newsboat"),
            &data_dir.join("Newsboat"));
    }
}
}
