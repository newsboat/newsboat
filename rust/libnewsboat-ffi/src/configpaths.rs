use crate::cliargsparser::CliArgsParser;
use crate::filepath::PathBuf;
use libnewsboat::configpaths;
use std::pin::Pin;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct ConfigPaths(configpaths::ConfigPaths);

#[cxx::bridge(namespace = "newsboat::configpaths::bridged")]
mod bridged {
    #[namespace = "newsboat::cliargsparser::bridged"]
    extern "C++" {
        include!("libnewsboat-ffi/src/cliargsparser.rs.h");
        type CliArgsParser = crate::cliargsparser::CliArgsParser;
    }
    #[namespace = "newsboat::filepath::bridged"]
    extern "C++" {
        include!("libnewsboat-ffi/src/filepath.rs.h");
        type PathBuf = crate::filepath::PathBuf;
    }

    extern "Rust" {
        type ConfigPaths;

        fn create() -> Box<ConfigPaths>;

        fn create_dirs(configpaths: &ConfigPaths) -> bool;

        fn initialized(configpaths: &ConfigPaths) -> bool;
        fn error_message(configpaths: &ConfigPaths) -> String;

        fn process_args(configpaths: &mut ConfigPaths, args: &CliArgsParser);

        fn try_migrate_from_newsbeuter(configpaths: &mut ConfigPaths) -> bool;

        fn url_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
        fn cache_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
        fn set_cache_file(configpaths: &mut ConfigPaths, path: &PathBuf);
        fn config_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
        fn lock_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
        fn queue_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
        fn search_history_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
        fn cmdline_history_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>);
    }
}

fn create() -> Box<ConfigPaths> {
    Box::new(ConfigPaths(configpaths::ConfigPaths::new()))
}

fn create_dirs(configpaths: &ConfigPaths) -> bool {
    configpaths.0.create_dirs()
}

fn initialized(configpaths: &ConfigPaths) -> bool {
    configpaths.0.initialized()
}

fn error_message(configpaths: &ConfigPaths) -> String {
    configpaths.0.error_message().to_owned()
}

fn process_args(configpaths: &mut ConfigPaths, args: &CliArgsParser) {
    configpaths.0.process_args(&args.0);
}

fn try_migrate_from_newsbeuter(configpaths: &mut ConfigPaths) -> bool {
    configpaths.0.try_migrate_from_newsbeuter()
}

fn url_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.url_file().to_owned();
}

fn cache_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.cache_file().to_owned();
}

fn set_cache_file(configpaths: &mut ConfigPaths, path: &PathBuf) {
    configpaths.0.set_cache_file(path.0.to_owned());
}

fn config_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.config_file().to_owned();
}

fn lock_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.lock_file().to_owned();
}

fn queue_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.queue_file().to_owned();
}

fn search_history_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.search_history_file().to_owned();
}

fn cmdline_history_file(configpaths: &ConfigPaths, mut path: Pin<&mut PathBuf>) {
    path.0 = configpaths.0.cmdline_history_file().to_owned();
}
