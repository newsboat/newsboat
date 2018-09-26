extern crate libc;

use libc::c_char;
use std::ffi::{CStr, CString};

pub mod utils;

#[no_mangle]
pub extern "C" fn rs_replace_all(
    input: *const c_char,
    from: *const c_char,
    to: *const c_char)
    -> *const c_char
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
    let result_ptr = result.as_ptr();
    std::mem::forget(result);
    result_ptr
}
