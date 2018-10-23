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
pub extern "C" fn rs_cstring_free(string: *mut c_char) {
    unsafe {
        if string.is_null() { return }
        CString::from_raw(string);
    };
}
