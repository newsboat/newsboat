use libc::c_char;
use libc::c_void;

#[no_mangle]
pub extern "C" fn rs_matcher_parse(expr: *const c_char) -> *const c_void {
    std::ptr::null()
}

#[no_mangle]
pub extern "C" fn rs_matcher_matches(expr: *const c_char) -> bool {
    false
}

#[no_mangle]
pub extern "C" fn rs_matcher_destroy(expr: *const c_char) {
}


