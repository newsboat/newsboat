use libnewsboat::utils;
use std::env;
use std::ffi::OsString;

#[test]
fn t_get_default_browser() {
    let key = String::from("BROWSER");
    let firefox = OsString::from("firefox");
    let opera = OsString::from("opera");
    let lynx = OsString::from("lynx");

    env::remove_var(&key);
    assert_eq!(utils::get_default_browser(), lynx);

    env::set_var(&key, &firefox);
    assert_eq!(utils::get_default_browser(), firefox);
    env::set_var(&key, &opera);
    assert_eq!(utils::get_default_browser(), opera);
}
