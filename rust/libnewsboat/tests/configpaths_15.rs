use libNewsboat::configpaths::ConfigPaths;
use section_testing::{enable_sections, section};
use std::{env, fs, path};
use tempfile::TempDir;

mod configpaths_helpers;

fn assert_not_migrated(
    urls_filepath: &path::Path,
    boat_sentries: &configpaths_helpers::FileSentries,
) {
    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());

    // No migration should occur, so should return false.
    assert!(!paths.try_migrate_from_newsbeuter());

    assert_eq!(
        configpaths_helpers::file_contents(urls_filepath),
        boat_sentries.urls
    );
}

enable_sections! {
#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_urls_file_already_exists(
) {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    if section!("Newsbeuter dotdir exists") {
        let _beuter_sentries = configpaths_helpers::mock_newsbeuter_dotdir(&tmp);

        if section!("Newsboat uses dotdir") {
            let boat_sentries = configpaths_helpers::mock_Newsboat_dotdir(&tmp);

            let urls_filepath = tmp.path().join(".Newsboat/").join("urls");
            assert_not_migrated(&urls_filepath, &boat_sentries);
        }

        if section!("Newsboat uses XDG") {
            if section!("Default XDG locations") {
                let boat_sentries = configpaths_helpers::mock_Newsboat_xdg_dirs(&tmp);

                let urls_filepath = tmp.path().join(".config").join("Newsboat").join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("XDG_CONFIG_HOME redefined") {
                let config_dir = tmp.path().join("xdg-conf");
                assert!(fs::create_dir(&config_dir).is_ok());
                env::set_var("XDG_CONFIG_HOME", &config_dir);

                let Newsboat_config_dir = config_dir.join("Newsboat");
                let Newsboat_data_dir = tmp.path().join(".local").join("share").join("Newsboat");

                let boat_sentries = configpaths_helpers::mock_xdg_dirs(
                    &Newsboat_config_dir,
                    &Newsboat_data_dir);

                let urls_filepath = Newsboat_config_dir.join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("XDG_DATA_HOME redefined") {
                let data_dir = tmp.path().join("xdg-data");
                assert!(fs::create_dir(&data_dir).is_ok());
                env::set_var("XDG_DATA_HOME", &data_dir);

                let Newsboat_config_dir =
                    tmp.path().join(".config").join("Newsboat");
                let Newsboat_data_dir = data_dir.join("Newsboat");

                let boat_sentries = configpaths_helpers::mock_xdg_dirs(
                    &Newsboat_config_dir,
                    &Newsboat_data_dir);

                let urls_filepath = Newsboat_config_dir.join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
                let config_dir = tmp.path().join("xdg-conf");
                assert!(fs::create_dir(&config_dir).is_ok());
                env::set_var("XDG_CONFIG_HOME", &config_dir);

                let data_dir = tmp.path().join("xdg-data");
                assert!(fs::create_dir(&data_dir).is_ok());
                env::set_var("XDG_DATA_HOME", &data_dir);

                let Newsboat_config_dir = config_dir.join("Newsboat");
                let Newsboat_data_dir = data_dir.join("Newsboat");

                let boat_sentries = configpaths_helpers::mock_xdg_dirs(
                    &Newsboat_config_dir,
                    &Newsboat_data_dir);

                let urls_filepath = Newsboat_config_dir.join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }
        }
    }

    if section!("Newsbeuter XDG dirs exist") {
        if section!("Default XDG locations") {
            let _beuter_sentries = configpaths_helpers::mock_newsbeuter_xdg_dirs(&tmp);

            if section!("Newsboat uses dotdir") {
                let boat_sentries = configpaths_helpers::mock_Newsboat_dotdir(&tmp);

                let urls_filepath = tmp.path().join(".Newsboat").join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("Newsboat uses XDG") {
                let boat_sentries = configpaths_helpers::mock_Newsboat_xdg_dirs(&tmp);

                let urls_filepath = tmp.path().join(".config").join("Newsboat").join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }
        }

        if section!("XDG_CONFIG_HOME redefined") {
            let config_dir = tmp.path().join("xdg-conf");
            assert!(fs::create_dir(&config_dir).is_ok());
            env::set_var("XDG_CONFIG_HOME", &config_dir);

            let _beuter_sentries = configpaths_helpers::mock_xdg_dirs(
                &config_dir.join("newsbeuter"),
                &tmp.path().join(".local").join("share").join("newsbeuter"));

            if section!("Newsboat uses dotdir") {
                let boat_sentries = configpaths_helpers::mock_Newsboat_dotdir(&tmp);

                let urls_filepath = tmp.path().join(".Newsboat").join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("Newsboat uses XDG") {
                let Newsboat_config_dir = config_dir.join("Newsboat");

                let boat_sentries = configpaths_helpers::mock_xdg_dirs(
                    &Newsboat_config_dir,
                    &tmp.path().join(".local").join("share").join("Newsboat"));

                let urls_filepath = Newsboat_config_dir.join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }
        }

        if section!("XDG_DATA_HOME redefined") {
            let data_dir = tmp.path().join("xdg-data");
            assert!(fs::create_dir(&data_dir).is_ok());
            env::set_var("XDG_DATA_HOME", &data_dir);

            let _beuter_sentries = configpaths_helpers::mock_xdg_dirs(
                &tmp.path().join(".config").join("newsbeuter"),
                &data_dir);

            if section!("Newsboat uses dotdir") {
                let boat_sentries = configpaths_helpers::mock_Newsboat_dotdir(&tmp);

                let urls_filepath = tmp.path().join(".Newsboat").join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("Newsboat uses XDG") {
                let Newsboat_config_dir = tmp.path().join(".config").join("Newsboat");

                let boat_sentries = configpaths_helpers::mock_xdg_dirs(
                    &Newsboat_config_dir,
                    &data_dir.join("Newsboat")
                    );

                let urls_filepath = Newsboat_config_dir.join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }
        }

        if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
            let config_dir = tmp.path().join("xdg-conf");
            assert!(fs::create_dir(&config_dir).is_ok());
            env::set_var("XDG_CONFIG_HOME", &config_dir);

            let data_dir = tmp.path().join("xdg-data");
            assert!(fs::create_dir(&data_dir).is_ok());
            env::set_var("XDG_DATA_HOME", &data_dir);

            let _beuter_sentries = configpaths_helpers::mock_xdg_dirs(
                &config_dir.join("newsbeuter"),
                &data_dir.join("newsbeuter"));

            if section!("Newsboat uses dotdir") {
                let boat_sentries = configpaths_helpers::mock_Newsboat_dotdir(&tmp);

                let urls_filepath = tmp.path().join(".Newsboat").join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }

            if section!("Newsboat uses XDG") {
                let Newsboat_config_dir = config_dir.join("Newsboat");

                let boat_sentries = configpaths_helpers::mock_xdg_dirs(
                    &Newsboat_config_dir,
                    &data_dir.join("Newsboat"));

                let urls_filepath = Newsboat_config_dir.join("urls");
                assert_not_migrated(&urls_filepath, &boat_sentries);
            }
        }
    }
}
}
