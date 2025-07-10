use libNewsboat::configpaths::ConfigPaths;
use std::env;
use std::fs;
use tempfile::TempDir;

#[test]
fn t_returns_paths_to_Newsboat_dotdir_if_no_Newsboat_dirs_exist() {
    let tmp = TempDir::new().unwrap();
    let Newsboat_dir = tmp.path().join(".Newsboat");
    fs::create_dir_all(&Newsboat_dir).ok();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    let paths = ConfigPaths::new();
    assert!(paths.initialized());
    assert_eq!(paths.url_file(), Newsboat_dir.join("urls"));
    assert_eq!(paths.cache_file(), Newsboat_dir.join("cache.db"));
    assert_eq!(paths.lock_file(), Newsboat_dir.join("cache.db.lock"));
    assert_eq!(paths.config_file(), Newsboat_dir.join("config"));
    assert_eq!(paths.queue_file(), Newsboat_dir.join("queue"));
    assert_eq!(
        paths.search_history_file(),
        Newsboat_dir.join("history.search")
    );
    assert_eq!(
        paths.cmdline_history_file(),
        Newsboat_dir.join("history.cmdline")
    );
}
