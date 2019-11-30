use libc::c_char;
use libc::c_void;
use std::ffi::CString;
use std::ffi::CStr;

use abort_on_panic;
use libnewsboat::matcher::{Matchable, Matcher};

extern "C" {
#[no_mangle]
fn matchable_has_attribute(item: *const c_void, attr: *const c_char) -> bool;
#[no_mangle]
fn matchable_get_attribute(item: *const c_void, attr: *const c_char) -> *mut c_char;
}

struct ForeignMatchable {
    ptr: *const c_void
}

impl Matchable for ForeignMatchable {
    fn has_attribute(&self, attr: &str) -> bool {
        unsafe {
            let attr = CString::new(attr).unwrap();
            matchable_has_attribute(self.ptr, attr.as_ptr())
        }
    }
    fn get_attribute(&self, attr: &str) -> String {
        unsafe {
            let attr = CString::new(attr).unwrap();
            let cpp_result = matchable_get_attribute(self.ptr, attr.as_ptr());
            let result = CStr::from_ptr(cpp_result).to_string_lossy().into_owned();
            libc::free(cpp_result as *mut c_void);
            result
        }
    }
}

#[no_mangle]
pub extern "C" fn rs_matcher_parse(expr: *const c_char) -> *const c_void {
    abort_on_panic( || {
        let expr = unsafe {
            CStr::from_ptr(expr).to_string_lossy().into_owned()
        };
        let res = Matcher::parse(&expr);
        match res {
            Ok(m) => {
                let m = Box::new(m);
                Box::into_raw(m) as *const c_void
            },
            Err(err) => std::ptr::null() // Ignore errors for now
        }
    })
}

#[no_mangle]
pub extern "C" fn rs_matcher_matches(matcher: *mut c_void, matchable: *const c_void) -> bool {
    abort_on_panic( || {
        let matcher = unsafe {
            Box::from_raw(matcher as *mut Matcher)
        };

        let matchable = ForeignMatchable {
            ptr: matchable
        };

        let ret = matcher.matches(matchable);

        // Deconstruct Box to avoid automatic deallocation as there's still a
        // pointer saved on the C++ side
        Box::into_raw(matcher) as *mut c_void;

        ret
    })
}

#[no_mangle]
pub extern "C" fn rs_matcher_destroy(matcher: *mut c_void) {
    abort_on_panic( || {
        //let matcher = unsafe {
        //    Box::from_raw(matcher as *mut Matcher)
        //};
        // Implicit deallocation
    })
}


