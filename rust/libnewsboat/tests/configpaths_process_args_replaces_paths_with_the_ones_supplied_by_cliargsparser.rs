extern crate libnewsboat;

use self::libnewsboat::{cliargsparser::CliArgsParser, configpaths::ConfigPaths};
use std::env;

#[test]
fn t_configpaths_process_args_replaces_paths_with_the_ones_supplied_by_cliargsparser() {
    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("HOME");
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    let url_file = "my urls file".to_string();
    let cache_file = "/path/to/cache file.db".to_string();
    let lock_file = cache_file.clone() + &".lock";
    let config_file = "this is a/config".to_string();

    let parser = CliArgsParser::new(vec![
        "newsboat".to_string(),
        "-u".to_string(),
        url_file.clone(),
        "-c".to_string(),
        cache_file.clone(),
        "-C".to_string(),
        config_file.clone(),
        "-q".to_string(),
    ]);

    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());
    paths.process_args(&parser);
    assert_eq!(paths.url_file(), url_file);
    assert_eq!(paths.cache_file(), cache_file);
    assert_eq!(paths.lock_file(), lock_file);
    assert_eq!(paths.config_file(), config_file);
}
