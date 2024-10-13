use libnewsboat::utils;
use serial_test::parallel;
use std::path::Path;

#[test]
#[parallel]
fn t_getcwd_returns_non_empty_string() {
    let result = utils::getcwd();
    assert!(result.is_ok());
    assert_ne!(result.unwrap(), Path::new(""));
}
