extern crate libnewsboat;

use libnewsboat::utils;
use std::env;

#[test]
fn t_resolve_tilde() {
    env::set_var("HOME", "test");
    assert_eq!(utils::resolve_tilde(String::from("~")), "test");
    assert_eq!(utils::resolve_tilde(String::from("~/")), "test/");
    assert_eq!(
        utils::resolve_tilde(String::from("~/foo/bar")),
        "test/foo/bar"
    );
    assert_eq!(utils::resolve_tilde(String::from("/foo/bar")), "/foo/bar");
}
