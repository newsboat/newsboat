use libnewsboat::configpaths::ConfigPaths;
use std::env;
use tempfile::TempDir;

mod configpaths_helpers;

#[test]
fn t_configpaths_try_migrate_from_newsbeuter_migrates_default_newsbeuter_dotdir_to_default_newsboat_dotdir(
) {
    let tmp = TempDir::new().unwrap();

    unsafe { env::set_var("HOME", tmp.path()) };

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    unsafe { env::remove_var("XDG_CONFIG_HOME") };
    unsafe { env::remove_var("XDG_DATA_HOME") };

    let sentries = configpaths_helpers::mock_newsbeuter_dotdir(&tmp);

    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());

    // Files should be migrated, so should return true.
    assert!(paths.try_migrate_from_newsbeuter());

    let dotdir = tmp.path().join(".newsboat");
    assert_eq!(
        &configpaths_helpers::file_contents(&dotdir.join("config")),
        &sentries.config
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&dotdir.join("urls")),
        &sentries.urls
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&dotdir.join("cache.db")),
        &sentries.cache
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&dotdir.join("queue")),
        &sentries.queue
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&dotdir.join("history.search")),
        &sentries.search
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&dotdir.join("history.cmdline")),
        &sentries.cmdline
    );
}
