extern crate libnewsboat;
#[macro_use]
extern crate section_testing;
extern crate tempfile;

use self::libnewsboat::configpaths::ConfigPaths;
use std::{env, path::PathBuf};

enable_sections! {
#[test]
fn t_configpaths_returns_possible_paths_to_urls_file() {
    env::set_var("HOME", "/absurd/homedir path");

    if section!("XDG_CONFIG_HOME is set") {
        env::set_var("XDG_CONFIG_HOME", "/absurd/homedir path/configurations");

        let paths = ConfigPaths::new();

        let expected_dotdir = PathBuf::from("/absurd/homedir path/.newsboat/urls");
        let expected_xdg = PathBuf::from("/absurd/homedir path/configurations/newsboat/urls");
        let expected = (expected_dotdir, expected_xdg);
        assert_eq!(paths.expected_urls_paths(), expected);
    }

    if section!("XDG_CONFIG_HOME is not set") {
        env::remove_var("XDG_CONFIG_HOME");

        let paths = ConfigPaths::new();

        let expected_dotdir = PathBuf::from("/absurd/homedir path/.newsboat/urls");
        let expected_xdg = PathBuf::from("/absurd/homedir path/.config/newsboat/urls");
        let expected = (expected_dotdir, expected_xdg);
        assert_eq!(paths.expected_urls_paths(), expected);
    }
}
}
