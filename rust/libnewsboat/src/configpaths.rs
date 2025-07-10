use crate::cliargsparser::CliArgsParser;
use crate::logger::{self, Level};
use crate::utils;
use gettextrs::gettext;
use std::fs::{self, DirBuilder};
use std::io;
use std::os::unix::fs::DirBuilderExt;
use std::path::{Path, PathBuf};
use strprintf::fmt;

pub const NEWSBOAT_SUBDIR_XDG: &str = "Newsboat";
pub const NEWSBOAT_CONFIG_SUBDIR: &str = ".Newsboat";
pub const NEWSBEUTER_SUBDIR_XDG: &str = "newsbeuter";
pub const NEWSBEUTER_CONFIG_SUBDIR: &str = ".newsbeuter";
pub const LOCK_SUFFIX: &str = ".lock";

const URLS_FILENAME: &str = "urls";
const CACHE_FILENAME: &str = "cache.db";
const CONFIG_FILENAME: &str = "config";
const QUEUE_FILENAME: &str = "queue";
const SEARCH_HISTORY_FILENAME: &str = "history.search";
const CMDLINE_HISTORY_FILENAME: &str = "history.cmdline";

#[derive(Debug, Default)]
pub struct ConfigPaths {
    /// Path to the user's home directory.
    ///
    /// Can be empty, in which case `error_message` should contain an explanation.
    env_home: PathBuf,

    /// An explanation why `env_home` is empty.
    error_message: String,

    /// Path to Newsboat's data directory.
    ///
    /// This can be ~/.Newsboat, or ~/.local/share/Newsboat, or something else entirely if user
    /// changed it through the command line parameter.
    data_dir: PathBuf,

    /// Path to Newsboat's configuration directory.
    ///
    /// This can be ~/.Newsboat, or ~/.config/Newsboat, or something else entirely if user changed
    /// it through the command line parameter.
    config_dir: PathBuf,

    url_file: PathBuf,
    cache_file: PathBuf,
    config_file: PathBuf,
    lock_file: PathBuf,
    queue_file: PathBuf,
    search_history_file: PathBuf,
    cmdline_history_file: PathBuf,

    silent: bool,
    using_nonstandard_configs: bool,
}

impl ConfigPaths {
    pub fn new() -> ConfigPaths {
        let mut config_paths = ConfigPaths {
            env_home: PathBuf::new(),
            error_message: String::new(),

            data_dir: PathBuf::new(),
            config_dir: PathBuf::new(),

            url_file: PathBuf::new(),
            cache_file: PathBuf::new(),
            config_file: PathBuf::new(),
            lock_file: PathBuf::new(),
            queue_file: PathBuf::new(),
            search_history_file: PathBuf::new(),
            cmdline_history_file: PathBuf::new(),

            silent: false,
            using_nonstandard_configs: false,
        };

        let env_home = utils::home_dir();
        if env_home.is_none() {
            let uid = unsafe { libc::getuid() };

            config_paths.error_message = fmt!(
                &gettext(
                    "Fatal error: couldn't determine home directory!\n\
                     Please set the HOME environment variable or add \
                     a valid user for UID %u!"
                ),
                uid
            );

            return config_paths;
        }

        // hitting this branch means we found a home directory
        // we can now safely call unwrap on all other functions in the dirs crate

        config_paths.env_home = env_home.unwrap();
        config_paths.find_dirs();
        config_paths
    }

    fn migrate_data_from_newsbeuter_xdg(&mut self) -> bool {
        // This can't panic because we've tested we can find the home directory in ConfigPaths::new
        // This should be replaced with proper error handling after this is not used by c++ anymore
        let xdg_dirs = xdg::BaseDirectories::new();
        let xdg_config_dir = xdg_dirs.get_config_home().unwrap();
        let xdg_data_dir = xdg_dirs.get_data_home().unwrap();

        let newsbeuter_config_dir = xdg_config_dir.join(NEWSBEUTER_SUBDIR_XDG);
        let newsbeuter_data_dir = xdg_data_dir.join(NEWSBEUTER_SUBDIR_XDG);

        let Newsboat_config_dir = xdg_config_dir.join(NEWSBOAT_SUBDIR_XDG);
        let Newsboat_data_dir = xdg_data_dir.join(NEWSBOAT_SUBDIR_XDG);

        if !newsbeuter_config_dir.is_dir() {
            return false;
        }

        fn exists(path: &Path) -> bool {
            let exists = path.exists();
            if exists {
                log!(
                    Level::Debug,
                    "{:?} already exists, aborting XDG migration.",
                    path
                );
            }
            exists
        }

        if exists(&Newsboat_config_dir) {
            return false;
        }

        if exists(&Newsboat_data_dir) {
            return false;
        }

        self.config_dir = Newsboat_config_dir;
        self.data_dir = Newsboat_data_dir;

        if !self.silent {
            eprintln!(
                "{}",
                &gettext("Migrating configs and data from Newsbeuter's XDG dirs...")
            );
        }

        if !try_mkdir(&self.config_dir) {
            return false;
        }

        if !try_mkdir(&self.data_dir) {
            return false;
        }

        // We ignore the return codes because it's okay if some files are missing.

        // in config
        let _ = migrate_file(&newsbeuter_config_dir, &self.config_dir, URLS_FILENAME);
        let _ = migrate_file(&newsbeuter_config_dir, &self.config_dir, CONFIG_FILENAME);

        // in data
        let _ = migrate_file(&newsbeuter_data_dir, &self.data_dir, CACHE_FILENAME);
        let _ = migrate_file(&newsbeuter_data_dir, &self.data_dir, QUEUE_FILENAME);
        let _ = migrate_file(
            &newsbeuter_data_dir,
            &self.data_dir,
            SEARCH_HISTORY_FILENAME,
        );
        let _ = migrate_file(
            &newsbeuter_data_dir,
            &self.data_dir,
            CMDLINE_HISTORY_FILENAME,
        );

        true
    }

    fn migrate_data_from_newsbeuter_simple(&self) -> bool {
        let newsbeuter_dir = self.env_home.join(NEWSBEUTER_CONFIG_SUBDIR);

        if !newsbeuter_dir.is_dir() {
            return false;
        }

        let Newsboat_dir = self.env_home.join(NEWSBOAT_CONFIG_SUBDIR);

        if Newsboat_dir.exists() {
            log!(
                Level::Debug,
                "{:?} already exists, aborting migration.",
                Newsboat_dir
            );
            return false;
        }

        if !self.silent {
            eprintln!(
                "{}",
                &gettext("Migrating configs and data from ~/.newsbeuter/...")
            );
        }

        match mkdir(&Newsboat_dir, 0o700) {
            Ok(_) => (),
            Err(ref e) if e.kind() == io::ErrorKind::AlreadyExists => (),
            Err(err) => {
                if !self.silent {
                    eprintln!(
                        "{}",
                        &fmt!(
                            &gettext("Aborting migration because mkdir on `%s' failed: %s"),
                            &Newsboat_dir.to_string_lossy().into_owned(),
                            err.to_string()
                        )
                    );
                }
                return false;
            }
        };

        // We ignore the return codes because it's okay if some files are missing.
        let _ = migrate_file(&newsbeuter_dir, &Newsboat_dir, URLS_FILENAME);
        let _ = migrate_file(&newsbeuter_dir, &Newsboat_dir, CACHE_FILENAME);
        let _ = migrate_file(&newsbeuter_dir, &Newsboat_dir, CONFIG_FILENAME);
        let _ = migrate_file(&newsbeuter_dir, &Newsboat_dir, QUEUE_FILENAME);
        let _ = migrate_file(&newsbeuter_dir, &Newsboat_dir, SEARCH_HISTORY_FILENAME);
        let _ = migrate_file(&newsbeuter_dir, &Newsboat_dir, CMDLINE_HISTORY_FILENAME);

        true
    }

    fn migrate_data_from_newsbeuter(&mut self) -> bool {
        let mut migrated = self.migrate_data_from_newsbeuter_xdg();

        if migrated {
            // Re-running to pick up XDG dirs
            self.find_dirs();
        } else {
            migrated = self.migrate_data_from_newsbeuter_simple();
        }

        migrated
    }

    pub fn create_dirs(&self) -> bool {
        try_mkdir(&self.config_dir) && try_mkdir(&self.data_dir)
    }

    fn find_dirs(&mut self) {
        self.config_dir = self.env_home.join(NEWSBOAT_CONFIG_SUBDIR);

        self.data_dir.clone_from(&self.config_dir);

        // Will change config_dir and data_dir to point to XDG if XDG
        // directories are available.
        self.find_dirs_xdg();

        // in config
        self.url_file = self.config_dir.join(URLS_FILENAME);
        self.config_file = self.config_dir.join(CONFIG_FILENAME);

        // in data
        self.cache_file = self.data_dir.join(CACHE_FILENAME);
        self.lock_file = self.data_dir.join(CACHE_FILENAME.to_owned() + LOCK_SUFFIX);
        self.queue_file = self.data_dir.join(QUEUE_FILENAME);
        self.search_history_file = self.data_dir.join(SEARCH_HISTORY_FILENAME);
        self.cmdline_history_file = self.data_dir.join(CMDLINE_HISTORY_FILENAME);
    }

    fn find_dirs_xdg(&mut self) {
        // This can't panic because we've tested we can find the home directory in ConfigPaths::new
        // This should be replaced with proper error handling after this is not used by c++ anymore
        let xdg_dirs = xdg::BaseDirectories::new();
        let config_dir = xdg_dirs
            .get_config_home()
            .unwrap()
            .join(NEWSBOAT_SUBDIR_XDG);
        let data_dir = xdg_dirs.get_data_home().unwrap().join(NEWSBOAT_SUBDIR_XDG);

        if !config_dir.is_dir() {
            return;
        }

        /* Invariant: config dir exists.
         *
         * At this point, we're confident we'll be using XDG. We don't check if
         * data dir exists, because if it doesn't we'll create it. */

        self.config_dir = config_dir;
        self.data_dir = data_dir;
    }

    /// Indicates if the object can be used.
    ///
    /// If this method returned `false`, the cause for initialization failure can be found using
    /// `error_message()`.
    pub fn initialized(&self) -> bool {
        !self.env_home.as_os_str().is_empty()
    }

    /// Returns explanation why initialization failed.
    ///
    /// \note You shouldn't call this unless `initialized()` returns `false`.
    pub fn error_message(&self) -> &str {
        &self.error_message
    }

    /// Initializes paths to config, cache etc. from CLI arguments.
    pub fn process_args(&mut self, args: &CliArgsParser) {
        if let Some(ref url_file) = args.url_file {
            self.url_file.clone_from(url_file);
        }

        if let Some(ref cache_file) = args.cache_file {
            self.cache_file.clone_from(cache_file);
        }

        if let Some(ref lock_file) = args.lock_file {
            self.lock_file.clone_from(lock_file);
        }

        if let Some(ref config_file) = args.config_file {
            self.config_file.clone_from(config_file);
        }

        if let Some(ref queue_file) = args.queue_file {
            self.queue_file.clone_from(queue_file);
        }

        if let Some(ref search_history_file) = args.search_history_file {
            self.search_history_file.clone_from(search_history_file);
        }

        if let Some(ref cmdline_history_file) = args.cmdline_history_file {
            self.cmdline_history_file.clone_from(cmdline_history_file);
        }

        self.silent = args.silent;
        self.using_nonstandard_configs = args.using_nonstandard_configs();
    }

    /// Migrate configs and data from Newsbeuter if they exist. Return `true` if migrated
    /// something, `false` otherwise.
    pub fn try_migrate_from_newsbeuter(&mut self) -> bool {
        if !self.using_nonstandard_configs && !&self.url_file.exists() {
            return self.migrate_data_from_newsbeuter();
        }

        // No migration occurred.
        false
    }

    /// Path to the URLs file.
    pub fn url_file(&self) -> &Path {
        &self.url_file
    }

    /// Path to the cache file.
    pub fn cache_file(&self) -> &Path {
        &self.cache_file
    }

    /// Sets path to the cache file.
    // FIXME: this is actually a kludge that lets Controller change the path
    // midway. That logic should be moved into ConfigPaths, and this method
    // removed.
    pub fn set_cache_file(&mut self, mut path: PathBuf) {
        path.clone_into(&mut self.cache_file);
        self.lock_file = {
            let current_extension = path
                .extension()
                .map(|p| p.to_string_lossy().into_owned())
                .unwrap_or_default();
            path.set_extension(current_extension + LOCK_SUFFIX);
            path
        };
    }

    /// Path to the config file.
    pub fn config_file(&self) -> &Path {
        &self.config_file
    }

    /// Path to the lock file.
    ///
    /// \note This changes when path to config file changes.
    pub fn lock_file(&self) -> &Path {
        &self.lock_file
    }

    /// \brief Path to the queue file.
    ///
    /// Queue file stores enqueued podcasts. It's written by Newsboat, and read by Podboat.
    pub fn queue_file(&self) -> &Path {
        &self.queue_file
    }

    /// Path to the file with previous search queries.
    pub fn search_history_file(&self) -> &Path {
        &self.search_history_file
    }

    /// Path to the file with command-line history.
    pub fn cmdline_history_file(&self) -> &Path {
        &self.cmdline_history_file
    }
}

fn try_mkdir<R: AsRef<Path>>(path: R) -> bool {
    utils::mkdir_parents(&path.as_ref(), 0o700).is_ok()
}

fn mkdir<R: AsRef<Path>>(path: R, mode: u32) -> io::Result<()> {
    DirBuilder::new().mode(mode).create(path.as_ref())
}

fn migrate_file<R: AsRef<Path>>(newsbeuter_dir: R, Newsboat_dir: R, file: &str) -> io::Result<()> {
    let input_filepath = newsbeuter_dir.as_ref().join(file);
    let output_filepath = Newsboat_dir.as_ref().join(file);
    eprintln!("{input_filepath:?} -> {output_filepath:?}");
    fs::copy(input_filepath, output_filepath).map(|_| ())
}
