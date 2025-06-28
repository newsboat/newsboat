use libnewsboat::configpaths::ConfigPaths;
use section_testing::{enable_sections, section};
use std::{env, fs, path};
use tempfile::TempDir;

fn assert_dirs_exist_after_create_dirs(dirs: &[&path::Path], tmp: &TempDir) {
    let paths = ConfigPaths::new();
    assert!(paths.initialized());

    let prefix = tmp.path().to_string_lossy().into_owned();
    assert!(paths.url_file().starts_with(&prefix));
    assert!(paths.config_file().starts_with(&prefix));
    assert!(paths.cache_file().starts_with(&prefix));
    assert!(paths.lock_file().starts_with(&prefix));
    assert!(paths.queue_file().starts_with(&prefix));
    assert!(paths.search_history_file().starts_with(&prefix));
    assert!(paths.cmdline_history_file().starts_with(&prefix));

    assert!(paths.create_dirs());

    for dir in dirs {
        println!("Checking if `{}' directory exists", &dir.display());
        assert!(dir.exists());
    }
}

enable_sections! {
#[test]
fn t_configpaths_create_dirs_returns_true_if_both_config_and_data_dirs_exist_now() {
    let tmp = TempDir::new().unwrap();
    unsafe { env::set_var("HOME", tmp.path()) };

    let dotdir = tmp.path().join(".newsboat");

    if section!("Using dotdir") {
        unsafe { env::remove_var("XDG_CONFIG_HOME") };
        unsafe { env::remove_var("XDG_DATA_HOME") };

        if section!("Dotdir didn't exist") {
            assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
        }

        if section!("Dotdir already existed") {
            assert!(fs::create_dir_all(&dotdir).is_ok());

            assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
        }
    }

    if section!("Using XDG dirs") {
        if section!("No XDG environment variables") {
            unsafe { env::remove_var("XDG_CONFIG_HOME") };
            unsafe { env::remove_var("XDG_DATA_HOME") };

            let config_dir = tmp.path().join(".config").join("newsboat");
            let data_dir = tmp.path().join(".local").join("share").join("newsboat");

            if section!("No dirs existed => dotdir created") {
                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Config dir existed, data dir didn't => data dir created") {
                assert!(fs::create_dir_all(&config_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }

            if section!("Data dir existed, config dir didn't => dotdir created") {
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Both config and data dir existed => just returns true") {
                assert!(fs::create_dir_all(&config_dir).is_ok());
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }
        }

        if section!("XDG_CONFIG_HOME redefined") {
            let config_home = tmp
                .path()
                .join("config")
                .join(fastrand::u32(..).to_string());
            unsafe { env::set_var("XDG_CONFIG_HOME", &config_home) };

            unsafe { env::remove_var("XDG_DATA_HOME") };

            let config_dir = config_home.join("newsboat");
            let data_dir = tmp.path().join(".local").join("share").join("newsboat");

            if section!("No dirs existed => dotdir created") {
                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Config dir existed, data dir didn't => data dir created") {
                assert!(fs::create_dir_all(&config_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }

            if section!("Data dir existed, config dir didn't => dotdir created") {
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Both config and data dir existed => just returns true") {
                assert!(fs::create_dir_all(&config_dir).is_ok());
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }
        }

        if section!("XDG_DATA_HOME redefined") {
            unsafe { env::remove_var("XDG_CONFIG_HOME") };

            let data_home = tmp
                .path()
                .join("data")
                .join(fastrand::u32(..).to_string());
            unsafe { env::set_var("XDG_DATA_HOME", &data_home) };

            let config_dir = tmp.path().join(".config").join("newsboat");
            let data_dir = data_home.join("newsboat");

            if section!("No dirs existed => dotdir created") {
                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Config dir existed, data dir didn't => data dir created") {
                assert!(fs::create_dir_all(&config_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }

            if section!("Data dir existed, config dir didn't => dotdir created") {
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Both config and data dir existed => just returns true") {
                assert!(fs::create_dir_all(&config_dir).is_ok());
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }
        }

        if section!("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
            let config_home = tmp
                .path()
                .join("config")
                .join(fastrand::u32(..).to_string());
            unsafe { env::set_var("XDG_CONFIG_HOME", &config_home) };

            let data_home = tmp
                .path()
                .join("data")
                .join(fastrand::u32(..).to_string());
            unsafe { env::set_var("XDG_DATA_HOME", &data_home) };

            let config_dir = config_home.join("newsboat");
            let data_dir = data_home.join("newsboat");

            if section!("No dirs existed => dotdir created") {
                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Config dir existed, data dir didn't => data dir created") {
                assert!(fs::create_dir_all(&config_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }

            if section!("Data dir existed, config dir didn't => dotdir created") {
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&dotdir], &tmp);
            }

            if section!("Both config and data dir existed => just returns true") {
                assert!(fs::create_dir_all(&config_dir).is_ok());
                assert!(fs::create_dir_all(&data_dir).is_ok());

                assert_dirs_exist_after_create_dirs(&[&config_dir, &data_dir], &tmp);
            }
        }
    }
}
}
