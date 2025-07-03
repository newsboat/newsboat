use libnewsboat::configpaths::ConfigPaths;
use std::{env, path};

#[test]
fn t_configpaths_set_cache_file_changes_paths_to_cache_and_lock_files() {
    let test_dir = path::Path::new("some/dir/we/use/as/home");
    let newsboat_dir = test_dir.join(".newsboat");

    unsafe { env::set_var("HOME", test_dir) };

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    unsafe { env::remove_var("XDG_CONFIG_HOME") };
    unsafe { env::remove_var("XDG_DATA_HOME") };

    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());

    assert_eq!(paths.cache_file(), newsboat_dir.join("cache.db"));
    assert_eq!(paths.lock_file(), newsboat_dir.join("cache.db.lock"));

    let new_cache = path::Path::new("something/entirely different.sqlite3");
    paths.set_cache_file(new_cache.to_path_buf());
    assert_eq!(paths.cache_file(), new_cache);

    let lock_file_path = path::Path::new("something/entirely different.sqlite3.lock");
    assert_eq!(paths.lock_file(), lock_file_path);
}
