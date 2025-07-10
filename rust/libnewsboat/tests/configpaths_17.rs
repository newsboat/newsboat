use libNewsboat::configpaths::ConfigPaths;
use section_testing::{enable_sections, section};
use std::{env, fs, path};
use tempfile::TempDir;

mod configpaths_helpers;

fn assert_files_migrated_to(
    config_dir: &path::Path,
    data_dir: &path::Path,
    sentries: &configpaths_helpers::FileSentries,
) {
    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());

    // Files should be migrated, so should return true.
    assert!(paths.try_migrate_from_newsbeuter());

    assert_eq!(
        &configpaths_helpers::file_contents(&config_dir.join("config")),
        &sentries.config
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&config_dir.join("urls")),
        &sentries.urls
    );

    assert_eq!(
        &configpaths_helpers::file_contents(&data_dir.join("cache.db")),
        &sentries.cache
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&data_dir.join("queue")),
        &sentries.queue
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&data_dir.join("history.search")),
        &sentries.search
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&data_dir.join("history.cmdline")),
        &sentries.cmdline
    );
}

enable_sections! {
#[test]
fn t_configpaths_try_migrate_from_newsbeuter_migrates_default_newsbeuter_xdg_dirs_to_default_Newsboat_xdg_dirs(
) {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    if section!("Default XDG locations")
    {
        let beuter_sentries = configpaths_helpers::mock_newsbeuter_xdg_dirs(&tmp);

        let config_dir = tmp.path().join(".config").join("Newsboat");
        let data_dir = tmp.path().join(".local").join("share").join("Newsboat");
        assert_files_migrated_to(&config_dir, &data_dir, &beuter_sentries);
    }

    if section!("XDG_CONFIG_HOME redefined") {
        let config_dir = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_dir).is_ok());
        env::set_var("XDG_CONFIG_HOME", &config_dir);

        let beuter_config_dir = &config_dir.join("newsbeuter");
        let beuter_data_dir = tmp.path().join(".local").join("share").join("newsbeuter");
        let beuter_sentries = configpaths_helpers::mock_xdg_dirs(beuter_config_dir, &beuter_data_dir);

        let boat_config_dir = config_dir.join("Newsboat");
        let boat_data_dir = tmp.path().join(".local").join("share").join("Newsboat");
        assert_files_migrated_to(&boat_config_dir, &boat_data_dir, &beuter_sentries);
    }

    if section!("XDG_DATA_HOME redefined") {
        let data_dir = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_dir).is_ok());
        env::set_var("XDG_DATA_HOME", &data_dir);

        let beuter_config_dir = tmp.path().join(".config").join("newsbeuter");
        let beuter_data_dir = &data_dir.join("newsbeuter");
        let beuter_sentries = configpaths_helpers::mock_xdg_dirs(&beuter_config_dir, beuter_data_dir);

        let boat_config_dir = tmp.path().join(".config").join("Newsboat");
        let boat_data_dir = data_dir.join("Newsboat");
        assert_files_migrated_to(&boat_config_dir, &boat_data_dir, &beuter_sentries);
    }

    if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
        let config_dir = tmp.path().join("xdg-conf");
        assert!(fs::create_dir(&config_dir).is_ok());
        env::set_var("XDG_CONFIG_HOME", &config_dir);

        let data_dir = tmp.path().join("xdg-data");
        assert!(fs::create_dir(&data_dir).is_ok());
        env::set_var("XDG_DATA_HOME", &data_dir);

        let beuter_config_dir = &config_dir.join("newsbeuter");
        let beuter_data_dir = &data_dir.join("newsbeuter");
        let beuter_sentries = configpaths_helpers::mock_xdg_dirs(beuter_config_dir, beuter_data_dir);

        let boat_config_dir = config_dir.join("Newsboat");
        let boat_data_dir = data_dir.join("Newsboat");
        assert_files_migrated_to(&boat_config_dir, &boat_data_dir, &beuter_sentries);
    }
}
}
