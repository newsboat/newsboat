// Each function in this crate is used in a single place: a corresponding C++ wrapper. It doesn't
// make sense to document these functions because they are basically an internal detail and don't
// stand on their own.
#![allow(clippy::missing_safety_doc)]

use libc::c_char;
use std::ffi::CString;
use std::panic::{catch_unwind, UnwindSafe};
use std::process::abort;

pub mod cliargsparser;
pub mod configpaths;
pub mod fmtstrformatter;
pub mod history;
pub mod human_panic;
pub mod logger;
pub mod matchererror;
pub mod utils;

/// Runs a Rust function, and if it panics, calls abort(); otherwise returns what function
/// returned.
fn abort_on_panic<F: FnOnce() -> R + UnwindSafe, R>(function: F) -> R {
    match catch_unwind(function) {
        Ok(result) => result,
        Err(_cause) => abort(),
    }
}

#[no_mangle]
pub unsafe extern "C" fn rs_cstring_free(string: *mut c_char) {
    abort_on_panic(|| {
        if string.is_null() {
            return;
        }
        CString::from_raw(string);
    })
}
