use libnewsboat::utils;
use std::env;

#[test]
fn t_get_default_browser() {
    let key = String::from("BROWSER");
    let firefox = String::from("firefox");
    let opera = String::from("opera");
    let lynx = String::from("lynx");

    unsafe { env::remove_var(&key) };
    assert_eq!(utils::get_default_browser(), lynx);

    unsafe { env::set_var(&key, &firefox) };
    assert_eq!(utils::get_default_browser(), firefox);
    unsafe { env::set_var(&key, &opera) };
    assert_eq!(utils::get_default_browser(), opera);
}
