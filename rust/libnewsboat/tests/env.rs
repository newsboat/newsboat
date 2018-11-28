extern crate libnewsboat;

use libnewsboat::utils;
use std::env;
    
#[test]
fn t_get_default_browser() {

    let key = String::from("BROWSER");
    let firefox = String::from("firefox");
    let opera = String::from("opera");
    let lynx = String::from("lynx");
    
    env::remove_var(&key);
    assert_eq!(utils::get_default_browser(),lynx);
    
    env::set_var(&key, &firefox);
    assert_eq!(utils::get_default_browser(), firefox);
    env::set_var(&key, &opera);
    assert_eq!(utils::get_default_browser(), opera);
}

#[test]
fn t_resolve_tilde() {

    env::set_var("HOME","test");    
    assert_eq!(utils::resolve_tilde(String::from("~")),"test");
    assert_eq!(utils::resolve_tilde(String::from("~/")),"test/");
    assert_eq!(utils::resolve_tilde(String::from("~/foo/bar")),"test/foo/bar");
    assert_eq!(utils::resolve_tilde(String::from("/foo/bar")),"/foo/bar");
}
