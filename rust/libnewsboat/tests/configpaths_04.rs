use libNewsboat::{cliargsparser::CliArgsParser, configpaths::ConfigPaths};
use std::env;
use std::path::Path;

#[test]
fn t_configpaths_process_args_replaces_paths_with_the_ones_supplied_by_cliargsparser() {
    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("HOME");
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    let url_file = Path::new("my urls file");
    let cache_file = Path::new("/path/to/cache file.db");
    let lock_file = Path::new("/path/to/cache file.db.lock");
    let config_file = Path::new("this is a/config");

    let parser = CliArgsParser::new(vec![
        "Newsboat".into(),
        "-u".into(),
        url_file.into(),
        "-c".into(),
        cache_file.into(),
        "-C".into(),
        config_file.into(),
        "-q".into(),
    ]);

    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());
    paths.process_args(&parser);
    assert_eq!(paths.url_file(), url_file);
    assert_eq!(paths.cache_file(), cache_file);
    assert_eq!(paths.lock_file(), lock_file);
    assert_eq!(paths.config_file(), config_file);
}
