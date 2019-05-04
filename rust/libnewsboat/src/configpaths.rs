use dirs;
use libc;
use logger::{self, Level};
use std::fs::{self, DirBuilder};
use std::io::{self, Read};
use std::os::unix::fs::DirBuilderExt;
use std::path::{Path, PathBuf};

pub const NEWSBOAT_SUBDIR_XDG: &str = "newsboat";
pub const NEWSBOAT_CONFIG_SUBDIR: &str = ".newsboat";
pub const NEWSBEUTER_SUBDIR_XDG: &str = "newsbeuter";
pub const NEWSBEUTER_CONFIG_SUBDIR: &str = ".newsbeuter";
pub const LOCK_SUFFIX: &str = ".lock";

#[derive(Debug)]
pub struct ConfigPaths {
    m_env_home: PathBuf,
    m_error_message: String,

    m_data_dir: PathBuf,
    m_config_dir: PathBuf,

    m_url_file: String,
    m_cache_file: String,
    m_config_file: String,
    m_lock_file: String,
    m_queue_file: String,
    m_search_file: String,
    m_cmdline_file: String,

    m_silent: bool,
    m_using_nonstandard_configs: bool,
}

impl ConfigPaths {
    pub fn new() -> ConfigPaths {
        let mut config_paths = ConfigPaths {
            m_env_home: PathBuf::new(),
            m_error_message: String::new(),

            m_data_dir: PathBuf::new(),
            m_config_dir: PathBuf::new(),

            m_url_file: String::from("urls"),
            m_cache_file: String::from("cache.db"),
            m_config_file: String::from("config"),
            m_lock_file: String::new(),
            m_queue_file: String::from("queue"),
            m_search_file: String::new(),
            m_cmdline_file: String::new(),

            m_silent: false,
            m_using_nonstandard_configs: false,
        };

        let m_env_home = dirs::home_dir();
        if m_env_home.is_none() {
            let uid = unsafe { libc::getuid() };

            // TODO: i18n
            config_paths.m_error_message = format!("Fatal error: couldn't determine home directory!\nPlease set the HOME environment variable or add a valid user for UID {}!", uid);

            return config_paths;
        }

        // hitting this branch means we found a home directory
        // we can now safely call unwrap on all other functions in the dirs crate

        config_paths.m_env_home = m_env_home.unwrap();
        config_paths.find_dirs();
        config_paths
    }

    fn migrate_data_from_newsbeuter_xdg(&mut self) -> bool {
        // This can't panic because we've tested we can find the home directory in ConfigPaths::new
        // This should be replaced with proper error handling after this is not used by c++ anymore
        let xdg_config_dir = dirs::config_dir().unwrap();
        let xdg_data_dir = dirs::data_dir().unwrap();

        let newsbeuter_config_dir = xdg_config_dir.join(NEWSBEUTER_SUBDIR_XDG);
        let newsbeuter_data_dir = xdg_data_dir.join(NEWSBEUTER_SUBDIR_XDG);

        let newsboat_config_dir = xdg_config_dir.join(NEWSBOAT_SUBDIR_XDG);
        let newsboat_data_dir = xdg_data_dir.join(NEWSBOAT_SUBDIR_XDG);

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
            return exists;
        }

        if exists(&newsboat_config_dir) {
            return false;
        }

        if exists(&newsboat_data_dir) {
            return false;
        }

        self.m_config_dir = newsboat_config_dir;
        self.m_data_dir = newsboat_data_dir;

        if !self.m_silent {
            eprintln!("Migrating configs and data from Newsbeuter's XDG dirs...");
        }

        if !try_mkdir(&self.m_config_dir) {
            return false;
        }

        if !try_mkdir(&self.m_data_dir) {
            return false;
        }

        // in config
        migrate_file(&newsbeuter_config_dir, &self.m_config_dir, "urls").is_ok();
        migrate_file(&newsbeuter_config_dir, &self.m_config_dir, "config").is_ok();

        // in data
        migrate_file(&newsbeuter_data_dir, &self.m_data_dir, "cache.db").is_ok();
        migrate_file(&newsbeuter_data_dir, &self.m_data_dir, "queue").is_ok();
        migrate_file(&newsbeuter_data_dir, &self.m_data_dir, "history.search").is_ok();
        migrate_file(&newsbeuter_data_dir, &self.m_data_dir, "history.cmdline").is_ok();

        return true;
    }

    fn migrate_data_from_newsbeuter_simple(&self) -> bool {
        let newsbeuter_dir = self.m_env_home.join(NEWSBEUTER_CONFIG_SUBDIR);

        if !newsbeuter_dir.is_dir() {
            return false;
        }

        let newsboat_dir = self.m_env_home.join(NEWSBOAT_CONFIG_SUBDIR);

        if newsboat_dir.exists() {
            log!(
                Level::Debug,
                "{:?} already exists, aborting migration.",
                newsboat_dir
            );
            return false;
        }

        if !self.m_silent {
            eprintln!("Migrating configs and data from Newsbeuter's dotdir...");
        }

        match mkdir(&newsboat_dir, 0o700) {
            Ok(_) => (),
            Err(ref e) if e.kind() == io::ErrorKind::AlreadyExists => (),
            Err(err) => {
                if !self.m_silent {
                    eprintln!(
                        "Aborting migration because mkdir on {:?} failed: {}",
                        newsboat_dir, err
                    );
                }
                return false;
            }
        };

        migrate_file(&newsbeuter_dir, &newsboat_dir, "urls").ok();
        migrate_file(&newsbeuter_dir, &newsboat_dir, "cache.db").ok();
        migrate_file(&newsbeuter_dir, &newsboat_dir, "config").ok();
        migrate_file(&newsbeuter_dir, &newsboat_dir, "queue").ok();
        migrate_file(&newsbeuter_dir, &newsboat_dir, "history.search").ok();
        migrate_file(&newsbeuter_dir, &newsboat_dir, "history.cmdline").ok();

        return true;
    }

    fn migrate_data_from_newsbeuter(&mut self) {
        let mut migrated = self.migrate_data_from_newsbeuter_xdg();

        if migrated {
            // Re-running to pick up XDG dirs
            self.m_url_file = "urls".into();
            self.m_cache_file = "cache.db".into();
            self.m_config_file = "config".into();
            self.m_queue_file = "queue".into();
            self.find_dirs();
        } else {
            migrated = self.migrate_data_from_newsbeuter_simple();
        }

        if migrated {
            eprintln!("\nPlease check the results and press Enter to continue.");
            // only wait for enter, ignore actual input
            let mut buf = [0; 16];
            io::stdin().read(&mut buf).ok();
        }
    }

    fn create_dirs(&self) -> bool {
        return try_mkdir(&self.m_config_dir) && try_mkdir(&self.m_data_dir);
    }

    fn find_dirs(&mut self) {
        self.m_config_dir = self.m_env_home.join(NEWSBOAT_CONFIG_SUBDIR);

        self.m_data_dir = self.m_config_dir.clone();

        // Will change config_dir and data_dir to point to XDG if XDG
        // directories are available.
        self.find_dirs_xdg();

        // in config
        self.m_url_file = self
            .m_config_dir
            .join(&self.m_url_file)
            .to_string_lossy()
            .into();
        self.m_config_file = self
            .m_config_dir
            .join(&self.m_config_file)
            .to_string_lossy()
            .into();

        // in data
        self.m_cache_file = self
            .m_data_dir
            .join(&self.m_cache_file)
            .to_string_lossy()
            .into();
        self.m_lock_file = self.m_cache_file.clone() + LOCK_SUFFIX;
        self.m_queue_file = self
            .m_data_dir
            .join(&self.m_queue_file)
            .to_string_lossy()
            .into();
        self.m_search_file = self
            .m_data_dir
            .join("history.search")
            .to_string_lossy()
            .into();
        self.m_cmdline_file = self
            .m_data_dir
            .join("history.cmdline")
            .to_string_lossy()
            .into();
    }

    fn find_dirs_xdg(&mut self) {
        // This can't panic because we've tested we can find the home directory in ConfigPaths::new
        // This should be replaced with proper error handling after this is not used by c++ anymore
        let config_dir = dirs::config_dir().unwrap().join(NEWSBOAT_SUBDIR_XDG);
        let data_dir = dirs::data_dir().unwrap().join(NEWSBOAT_SUBDIR_XDG);

        if !config_dir.is_dir() {
            return;
        }

        /* Invariant: config dir exists.
         *
         * At this point, we're confident we'll be using XDG. We don't check if
         * data dir exists, because if it doesn't we'll create it. */

        self.m_config_dir = config_dir;
        self.m_data_dir = data_dir;
    }

    /// Indicates if the object can be used.
    ///
    /// If this method returned `false`, the cause for initialization failure can be found using
    /// `error_message()`.
    pub fn initialized(&self) -> bool {
        !self.m_env_home.as_os_str().is_empty()
    }

    /// Returns explanation why initialization failed.
    ///
    /// \note You shouldn't call this unless `initialized()` returns `false`.
    pub fn error_message(&self) -> &str {
        &self.m_error_message
    }

    // TODO
    /*
    /// Initializes paths to config, cache etc. from CLI arguments.
    pub fn process_args(const CliArgsParser& args) {
        unimplemented!()
    }
    */

    /// Ensures all directories exist (migrating them from Newsbeuter if possible).
    pub fn setup_dirs(&mut self) -> bool {
        if !self.m_using_nonstandard_configs && !Path::new(&self.m_url_file).exists() {
            self.migrate_data_from_newsbeuter();
        }

        self.create_dirs()
    }

    /// Path to the URLs file.
    pub fn url_file(&self) -> String {
        self.m_url_file.clone()
    }

    /// Path to the cache file.
    pub fn cache_file(&self) -> String {
        self.m_cache_file.clone()
    }

    /// Sets path to the cache file.
    // FIXME: this is actually a kludge that lets Controller change the path
    // midway. That logic should be moved into ConfigPaths, and this method
    // removed.
    pub fn set_cache_file(&mut self, path: String) {
        self.m_cache_file = path.clone();
        self.m_lock_file = path + LOCK_SUFFIX;
    }

    /// Path to the config file.
    pub fn config_file(&self) -> String {
        self.m_config_file.clone()
    }

    /// Path to the lock file.
    ///
    /// \note This changes when path to config file changes.
    pub fn lock_file(&self) -> String {
        self.m_lock_file.clone()
    }

    /// \brief Path to the queue file.
    ///
    /// Queue file stores enqueued podcasts. It's written by Newsboat, and read by Podboat.
    pub fn queue_file(&self) -> String {
        self.m_queue_file.clone()
    }

    /// Path to the file with previous search queries.
    pub fn search_file(&self) -> String {
        self.m_search_file.clone()
    }

    /// Path to the file with command-line history.
    pub fn cmdline_file(&self) -> String {
        self.m_cmdline_file.clone()
    }
}

fn try_mkdir<R: AsRef<Path>>(path: R) -> bool {
    DirBuilder::new()
        .recursive(true)
        .mode(0o700)
        .create(path.as_ref())
        .is_ok()
}

fn mkdir<R: AsRef<Path>>(path: R, mode: u32) -> io::Result<()> {
    DirBuilder::new().mode(mode).create(path.as_ref())
}

fn migrate_file<R: AsRef<Path>>(newsbeuter_dir: R, newsboat_dir: R, file: &str) -> io::Result<()> {
    let input_filepath = newsbeuter_dir.as_ref().join(file);
    let output_filepath = newsboat_dir.as_ref().join(file);
    eprintln!("{:?} -> {:?}", input_filepath, output_filepath);
    fs::copy(input_filepath, output_filepath).map(|_| ())
}
