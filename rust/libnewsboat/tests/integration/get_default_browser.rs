use libnewsboat::utils;
use serial_test::serial;
use std::env;

#[test]
#[serial] // Because it changes environment variables
fn t_get_default_browser() {
    let key = String::from("BROWSER");
    let firefox = String::from("firefox");
    let opera = String::from("opera");
    let lynx = String::from("lynx");

    env::remove_var(&key);
    assert_eq!(utils::get_default_browser(), lynx);

    env::set_var(&key, &firefox);
    assert_eq!(utils::get_default_browser(), firefox);
    env::set_var(&key, &opera);
    assert_eq!(utils::get_default_browser(), opera);
}
