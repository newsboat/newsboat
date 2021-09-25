use libnewsboat::cliargsparser;
use libnewsboat::configpaths;
use std::path::Path;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct ConfigPaths(configpaths::ConfigPaths);

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
pub struct FfiCliArgsParser(cliargsparser::CliArgsParser);

#[cxx::bridge(namespace = "newsboat::configpaths::bridged")]
mod bridged {
    extern "Rust" {
        type ConfigPaths;
        type FfiCliArgsParser;

        fn create() -> Box<ConfigPaths>;

        fn create_dirs(configpaths: &ConfigPaths) -> bool;

        fn initialized(configpaths: &ConfigPaths) -> bool;
        fn error_message(configpaths: &ConfigPaths) -> String;

        fn process_args(configpaths: &mut ConfigPaths, args: &FfiCliArgsParser);

        fn try_migrate_from_newsbeuter(configpaths: &mut ConfigPaths) -> bool;

        fn url_file(configpaths: &ConfigPaths) -> String;
        fn cache_file(configpaths: &ConfigPaths) -> String;
        fn set_cache_file(configpaths: &mut ConfigPaths, path: &str);
        fn config_file(configpaths: &ConfigPaths) -> String;
        fn lock_file(configpaths: &ConfigPaths) -> String;
        fn queue_file(configpaths: &ConfigPaths) -> String;
        fn search_file(configpaths: &ConfigPaths) -> String;
        fn cmdline_file(configpaths: &ConfigPaths) -> String;
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

fn process_args(configpaths: &mut ConfigPaths, args: &FfiCliArgsParser) {
    configpaths.0.process_args(&args.0);
}

fn try_migrate_from_newsbeuter(configpaths: &mut ConfigPaths) -> bool {
    configpaths.0.try_migrate_from_newsbeuter()
}

fn url_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.url_file().to_string_lossy().into_owned()
}

fn cache_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.cache_file().to_string_lossy().into_owned()
}

fn set_cache_file(configpaths: &mut ConfigPaths, path: &str) {
    configpaths.0.set_cache_file(Path::new(path).to_owned());
}

fn config_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.config_file().to_string_lossy().into_owned()
}

fn lock_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.lock_file().to_string_lossy().into_owned()
}

fn queue_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.queue_file().to_string_lossy().into_owned()
}

fn search_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.search_file().to_string_lossy().into_owned()
}

fn cmdline_file(configpaths: &ConfigPaths) -> String {
    configpaths.0.cmdline_file().to_string_lossy().into_owned()
}
