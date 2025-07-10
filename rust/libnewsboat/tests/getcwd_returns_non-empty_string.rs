use libNewsboat::utils;
use std::path::Path;

#[test]
fn t_getcwd_returns_non_empty_string() {
    let result = utils::getcwd();
    assert!(result.is_ok());
    assert_ne!(result.unwrap(), Path::new(""));
}
