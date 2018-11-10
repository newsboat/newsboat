extern crate libc;
extern crate libnewsboat;

use libc::c_char;
use std::ffi::{CStr, CString};

use libnewsboat::utils;

#[no_mangle]
pub extern "C" fn rs_replace_all(
    input: *const c_char,
    from: *const c_char,
    to: *const c_char)
    -> *mut c_char
{
    let rs_input = unsafe { CStr::from_ptr(input) };
    let rs_input = rs_input.to_string_lossy().into_owned();

    let rs_from = unsafe { CStr::from_ptr(from) };
    let rs_from = rs_from.to_str()
        .expect("rs_from contained invalid UTF-8");

    let rs_to = unsafe { CStr::from_ptr(to) };
    let rs_to = rs_to.to_str()
        .expect("rs_to contained invalid UTF-8");

    let result = utils::replace_all(rs_input, rs_from, rs_to);
    // Panic here can't happen because:
    // 1. panic can only happen if `result` contains null bytes;
    // 2. `result` contains what `input` contained, plus maybe what `to` contains (if there were
    //    replacements);
    // 3. neither `input` nor `to` could contain null bytes because they're null-terminated
    //    strings we got from C.
    let result = CString::new(result).unwrap();
    result.into_raw()
}

#[no_mangle]
pub extern "C" fn rs_consolidate_whitespace( input: *const c_char) -> *mut c_char {
    let rs_input = unsafe { CStr::from_ptr(input) };
    let rs_input = rs_input.to_string_lossy().into_owned();

    let result = utils::consolidate_whitespace(rs_input);
    // Panic here can't happen because:
    // 1. panic can only happen if `result` contains null bytes;
    // 2. `result` contains what `input` contained, and input is a
    // null-terminated string from C.
    let result = CString::new(result).unwrap();
    result.into_raw()
}

#[no_mangle]
pub extern "C" fn rs_to_u(in_str: *const c_char, default_value: u32) -> u32
{
    let rs_str = unsafe { CStr::from_ptr(in_str) };
    let rs_str = rs_str.to_string_lossy().into_owned();

    utils::to_u( rs_str, default_value)
}

#[no_mangle]
pub extern "C" fn rs_is_http_url(in_str: *const c_char) -> bool {
    let rs_str = unsafe { CStr::from_ptr(in_str) };
    let rs_str = rs_str.to_string_lossy();
    utils::is_http_url(&rs_str)
}

#[no_mangle]
pub extern "C" fn rs_is_query_url(in_str: *const c_char) -> bool {
    let rs_str = unsafe { CStr::from_ptr(in_str) };
    let rs_str = rs_str.to_string_lossy();
    utils::is_query_url(&rs_str)
}

#[no_mangle]
pub extern "C" fn rs_is_filter_url(in_str: *const c_char) -> bool {
    let rs_str = unsafe { CStr::from_ptr(in_str) };
    let rs_str = rs_str.to_string_lossy();
    utils::is_filter_url(&rs_str)
}

#[no_mangle]
pub extern "C" fn rs_trim_end(input: *const c_char) -> *mut c_char {
    let rs_input = unsafe { CStr::from_ptr(input) };
    let rs_input = rs_input.to_string_lossy().into_owned();

    let result = utils::trim_end(rs_input);

    let result = CString::new(result).unwrap();
    result.into_raw()
}

#[no_mangle]
pub extern "C" fn rs_trim(input: *const c_char) -> *mut c_char {
    let rs_input = unsafe { CStr::from_ptr(input) };
    let rs_input = rs_input.to_string_lossy().into_owned();

    let result = utils::trim(rs_input);

    let result = CString::new(result).unwrap();
    result.into_raw()
}

#[no_mangle]
pub extern "C" fn rs_get_random_value(rs_max: u32 ) -> u32 {
    utils::get_random_value(rs_max)
}

#[no_mangle]
pub extern "C" fn rs_cstring_free(string: *mut c_char) {
    unsafe {
        if string.is_null() { return }
        CString::from_raw(string);
    };
}

#[no_mangle]
pub extern "C" fn rs_get_default_browser() -> *mut c_char {
    let browser = utils::get_default_browser();
    // Panic here can't happen because:
    //1. panic can only happen if `result` contains null bytes; 
    //2. std::env::var returns an error if the variable contains invalid Unicode, 
    // In that case get_default_browser will return "lynx", which obviously doesn't contain null bytes.
    let result = CString::new(browser).unwrap();
    result.into_raw()
}

#[no_mangle]
pub extern "C" fn rs_is_valid_color(input: *const c_char) -> bool {
    let rs_input = unsafe { CStr::from_ptr(input) };
    let rs_input = rs_input.to_string_lossy();
    utils::is_valid_color(&rs_input)
}
