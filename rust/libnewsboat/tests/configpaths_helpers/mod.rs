// Each integration test uses only a handful of the functions here, and generates the "unused code"
// warnings for the rest. This is annoying and useless, so we suppress the warning.
#![allow(dead_code)]

// `libc` contains file mode constants, like S_IXUSR, which are useful for `struct Chmod`. Thus it
// makes sense to re-export the crate.
pub use libc;

use libNewsboat::configpaths::ConfigPaths;
use std::io::{Read, Write};
use std::os::unix::fs::PermissionsExt;
use std::{fs, path};
use tempfile::TempDir;

/// Strings that are placed in files before running commands, to see if commands modify those
/// files.
pub struct FileSentries {
    /// Sentry for the config file.
    pub config: String,
    /// Sentry for the urls file.
    pub urls: String,
    /// Sentry for the cache file.
    pub cache: String,
    /// Sentry for the queue file.
    pub queue: String,
    /// Sentry for the search history file.
    pub search: String,
    /// Sentry for the command-line history file.
    pub cmdline: String,
}

impl FileSentries {
    /// Create a new struct with random strings for sentries.
    pub fn new() -> FileSentries {
        FileSentries {
            config: fastrand::u32(..).to_string() + "config",
            urls: fastrand::u32(..).to_string() + "urls",
            cache: fastrand::u32(..).to_string() + "cache",
            queue: fastrand::u32(..).to_string() + "queue",
            search: fastrand::u32(..).to_string() + "search",
            cmdline: fastrand::u32(..).to_string() + "cmdline",
        }
    }
}

fn mock_dotdir(dotdir_path: &path::Path, sentries: &FileSentries) {
    assert!(fs::create_dir(dotdir_path).is_ok());
    assert!(create_file(&dotdir_path.join("config"), &sentries.config));
    assert!(create_file(&dotdir_path.join("urls"), &sentries.urls));
    assert!(create_file(&dotdir_path.join("cache.db"), &sentries.cache));
    assert!(create_file(&dotdir_path.join("queue"), &sentries.queue));
    assert!(create_file(
        &dotdir_path.join("history.search"),
        &sentries.search
    ));
    assert!(create_file(
        &dotdir_path.join("history.cmdline"),
        &sentries.cmdline
    ));
}

pub fn mock_newsbeuter_dotdir(tmp: &TempDir) -> FileSentries {
    let sentries = FileSentries::new();

    let dotdir_path = tmp.path().join(".newsbeuter");
    mock_dotdir(&dotdir_path, &sentries);

    sentries
}

pub fn mock_Newsboat_dotdir(tmp: &TempDir) -> FileSentries {
    let sentries = FileSentries::new();

    let dotdir_path = tmp.path().join(".Newsboat");
    mock_dotdir(&dotdir_path, &sentries);

    sentries
}

pub fn mock_xdg_dirs(config_dir: &path::Path, data_dir: &path::Path) -> FileSentries {
    let sentries = FileSentries::new();

    assert!(fs::create_dir_all(config_dir).is_ok());
    assert!(create_file(&config_dir.join("config"), &sentries.config));
    assert!(create_file(&config_dir.join("urls"), &sentries.urls));

    assert!(fs::create_dir_all(data_dir).is_ok());
    assert!(create_file(&data_dir.join("cache.db"), &sentries.cache));
    assert!(create_file(&data_dir.join("queue"), &sentries.queue));
    assert!(create_file(
        &data_dir.join("history.search"),
        &sentries.search
    ));
    assert!(create_file(
        &data_dir.join("history.cmdline"),
        &sentries.cmdline
    ));

    sentries
}

pub fn mock_newsbeuter_xdg_dirs(tmp: &TempDir) -> FileSentries {
    let config_dir_path = tmp.path().join(".config").join("newsbeuter");
    let data_dir_path = tmp.path().join(".local").join("share").join("newsbeuter");
    mock_xdg_dirs(&config_dir_path, &data_dir_path)
}

pub fn mock_Newsboat_xdg_dirs(tmp: &TempDir) -> FileSentries {
    let config_dir_path = tmp.path().join(".config").join("Newsboat");
    let data_dir_path = tmp.path().join(".local").join("share").join("Newsboat");
    mock_xdg_dirs(&config_dir_path, &data_dir_path)
}

/// Creates a file at given `filepath` and writes `content` into it.
///
/// Returns `true` if successful, `false` otherwise.
pub fn create_file(path: &path::Path, content: &str) -> bool {
    fs::File::create(path)
        .and_then(|mut f| f.write_all(content.as_bytes()))
        .is_ok()
}

pub fn file_contents(path: &path::Path) -> String {
    fs::File::open(path)
        .map(|mut f| {
            let mut buf = String::new();
            let _ = f.read_to_string(&mut buf);
            buf
        })
        // If failed to open/read file, return an empty string
        .unwrap_or_else(|_| String::new())
}

fn is_readable(filepath: &path::Path) -> bool {
    fs::File::open(filepath).is_ok()
}

pub fn assert_xdg_not_migrated(config_dir: &path::Path, data_dir: &path::Path) {
    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());

    // Shouldn't migrate anything, so should return false.
    assert!(!paths.try_migrate_from_newsbeuter());

    assert!(!is_readable(&config_dir.join("config")));
    assert!(!is_readable(&config_dir.join("urls")));

    assert!(!is_readable(&data_dir.join("cache.db")));
    assert!(!is_readable(&data_dir.join("queue")));
    assert!(!is_readable(&data_dir.join("history.search")));
    assert!(!is_readable(&data_dir.join("history.cmdline")));
}

/// Sets new permissions on a given path, and restores them back when the
/// object is destroyed.
pub struct Chmod<'a> {
    path: &'a path::Path,
    original_mode: u32,
}

impl<'a> Chmod<'a> {
    pub fn new(path: &'a path::Path, new_mode: libc::mode_t) -> Chmod<'a> {
        let original_mode = fs::metadata(path)
            .unwrap_or_else(|_| panic!("Chmod: couldn't obtain metadata for `{}'", path.display()))
            .permissions()
            .mode();

        // `from_mode` takes `u32`, but `libc::mode_t` is either `u16` (macOS, FreeBSD) or `u32`
        // (Linux). Suppress the warning to prevent Clippy on Linux from complaining.
        #[allow(clippy::useless_conversion)]
        fs::set_permissions(path, fs::Permissions::from_mode(new_mode.into()))
            .unwrap_or_else(|_| panic!("Chmod: couldn't change mode for `{}'", path.display()));

        Chmod {
            path,
            original_mode,
        }
    }
}

impl Drop for Chmod<'_> {
    fn drop(&mut self) {
        fs::set_permissions(self.path, fs::Permissions::from_mode(self.original_mode))
            .unwrap_or_else(|_| {
                panic!(
                    "Chmod: couldn't change back the mode for `{}'",
                    self.path.display()
                )
            });
    }
}

pub fn assert_dotdir_not_migrated(dotdir: &path::Path) {
    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());

    // Shouldn't migrate anything, so should return false.
    assert!(!paths.try_migrate_from_newsbeuter());

    assert!(!is_readable(&dotdir.join("config")));
    assert!(!is_readable(&dotdir.join("urls")));

    assert!(!is_readable(&dotdir.join("cache.db")));
    assert!(!is_readable(&dotdir.join("queue")));
    assert!(!is_readable(&dotdir.join("history.search")));
    assert!(!is_readable(&dotdir.join("history.cmdline")));
}

pub fn assert_create_dirs_returns_false(tmp: &TempDir) {
    let readonly = libc::S_IRUSR | libc::S_IXUSR;
    let _home_chmod = Chmod::new(tmp.path(), readonly);

    let paths = ConfigPaths::new();
    assert!(paths.initialized());
    assert!(!paths.create_dirs());
}
