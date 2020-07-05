use crate::abort_on_panic;
use libc::{c_char, c_void};
use libnewsboat::scopemeasure::ScopeMeasure;
use std::ffi::CStr;
use std::mem;

#[no_mangle]
pub unsafe extern "C" fn create_rs_scopemeasure(scope_name: *const c_char) -> *mut c_void {
    abort_on_panic(|| {
        let scope_name = CStr::from_ptr(scope_name).to_string_lossy().into_owned();
        Box::into_raw(Box::new(ScopeMeasure::new(scope_name))) as *mut c_void
    })
}

#[no_mangle]
pub unsafe extern "C" fn destroy_rs_scopemeasure(object: *mut c_void) {
    abort_on_panic(|| {
        if object.is_null() {
            return;
        }

        // Drop the object
        Box::from_raw(object as *mut ScopeMeasure);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_scopemeasure_stopover(
    object: *mut c_void,
    stopover_name: *const c_char,
) {
    abort_on_panic(|| {
        if object.is_null() {
            return;
        }

        let stopover_name = CStr::from_ptr(stopover_name)
            .to_str()
            .expect("stopover_name contained invalid UTF-8");

        let object = Box::from_raw(object as *mut ScopeMeasure);
        object.stopover(stopover_name);

        // Don't destroy the object when the function finishes
        mem::forget(object);
    })
}
