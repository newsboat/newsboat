use libNewsboat::{cliargsparser::CliArgsParser, configpaths::ConfigPaths};
use section_testing::{enable_sections, section};
use std::env;
use tempfile::TempDir;

mod configpaths_helpers;

enable_sections! {
#[test]
fn t_configpaths_try_migrate_from_newsbeuter_does_not_migrate_if_config_paths_were_specified_via_cli(
) {
    let tmp = TempDir::new().unwrap();

    env::set_var("HOME", tmp.path());

    // ConfigPaths rely on these variables, so let's sanitize them to ensure
    // that the tests aren't affected
    env::remove_var("XDG_CONFIG_HOME");
    env::remove_var("XDG_DATA_HOME");

    if section!("Newsbeuter dotdir exists")
    {
        configpaths_helpers::mock_newsbeuter_dotdir(&tmp);
    }

    if section!("Newsbeuter XDG dirs exist")
    {
        configpaths_helpers::mock_newsbeuter_xdg_dirs(&tmp);
    }

    let boat_sentries = configpaths_helpers::FileSentries::new();

    let url_file = tmp.path().join("my urls file");
    assert!(configpaths_helpers::create_file(
        &url_file,
        &boat_sentries.urls
    ));

    let cache_file = tmp.path().join("new cache.db");
    assert!(configpaths_helpers::create_file(
        &cache_file,
        &boat_sentries.cache
    ));

    let config_file = tmp.path().join("custom config file");
    assert!(configpaths_helpers::create_file(
        &config_file,
        &boat_sentries.config
    ));

    let parser = CliArgsParser::new(vec![
        "Newsboat".into(),
        "-u".into(),
        url_file.clone().into(),
        "-c".into(),
        cache_file.clone().into(),
        "-C".into(),
        config_file.clone().into(),
        "-q".into(),
    ]);

    let mut paths = ConfigPaths::new();
    assert!(paths.initialized());
    paths.process_args(&parser);

    // No migration should occur, so should return false.
    assert!(!paths.try_migrate_from_newsbeuter());

    assert_eq!(
        &configpaths_helpers::file_contents(&url_file),
        &boat_sentries.urls
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&config_file),
        &boat_sentries.config
    );
    assert_eq!(
        &configpaths_helpers::file_contents(&cache_file),
        &boat_sentries.cache
    );
}
}
