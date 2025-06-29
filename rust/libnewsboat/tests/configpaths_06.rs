use libnewsboat::configpaths::ConfigPaths;
use section_testing::{enable_sections, section};
use std::{env, fs, path};
use tempfile::TempDir;

fn assert_paths_are_inside_dirs(config_dir: &path::Path, data_dir: &path::Path) {
    let paths = ConfigPaths::new();
    assert!(paths.initialized());
    assert_eq!(paths.config_file(), config_dir.join("config"));
    assert_eq!(paths.url_file(), config_dir.join("urls"));

    assert_eq!(paths.cache_file(), data_dir.join("cache.db"));
    assert_eq!(paths.lock_file(), data_dir.join("cache.db.lock"));
    assert_eq!(paths.queue_file(), data_dir.join("queue"));
    assert_eq!(paths.search_history_file(), data_dir.join("history.search"));
    assert_eq!(
        paths.cmdline_history_file(),
        data_dir.join("history.cmdline")
    );
}

enable_sections! {
#[test]
fn t_configpaths_returns_paths_to_newsboat_xdg_dirs_if_they_exist_and_dotdir_doesnt()
{
    let tmp = TempDir::new().unwrap();
    let config_dir = tmp.path().join(".config").join("newsboat");
    fs::create_dir_all(&config_dir).ok();
    let data_dir = tmp.path().join(".local").join("share").join("newsboat");
    fs::create_dir_all(&data_dir).ok();

    unsafe { env::set_var("HOME", tmp.path()) };

    if section!("XDG_CONFIG_HOME is set") {
        unsafe { env::set_var("XDG_CONFIG_HOME", tmp.path().join(".config")) };

        if section!("XDG_DATA_HOME is set") {
            unsafe { env::set_var("XDG_DATA_HOME", tmp.path().join(".local").join("share")) };
            assert_paths_are_inside_dirs(&config_dir, &data_dir);
        }

        if section!("XDG_DATA_HOME is not set") {
            unsafe { env::remove_var("XDG_DATA_HOME") };
            assert_paths_are_inside_dirs(&config_dir, &data_dir);
        }
    }

    if section!("XDG_CONFIG_HOME is not set") {
        unsafe { env::remove_var("XDG_CONFIG_HOME") };

        if section!("XDG_DATA_HOME is set") {
            unsafe { env::set_var("XDG_DATA_HOME", tmp.path().join(".local").join("share")) };
            assert_paths_are_inside_dirs(&config_dir, &data_dir);
        }

        if section!("XDG_DATA_HOME is not set") {
            unsafe { env::remove_var("XDG_DATA_HOME") };
            assert_paths_are_inside_dirs(&config_dir, &data_dir);
        }
    }
}
}
