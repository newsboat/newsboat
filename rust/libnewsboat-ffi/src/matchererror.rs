use crate::abort_on_panic;
use libc::c_char;
use libNewsboat::matchererror::MatcherError;
use std::ffi::CString;
use std::ptr;

// These constants MUST match numbers in `enum MatcherException::Type`, see include/matcherexception.h
const ATTRIB_UNAVAIL: u8 = 0;
const INVALID_REGEX: u8 = 1;

#[repr(C)]
pub struct MatcherErrorFfi {
    err_type: u8, // One of the constants defined above
    info: *mut c_char,
    info2: *mut c_char,
}

#[no_mangle]
pub fn matcher_error_to_ffi(error: MatcherError) -> MatcherErrorFfi {
    abort_on_panic(|| {
        match error {
            MatcherError::AttributeUnavailable { attr } => {
                // NUL bytes in filter expressions are disallowed by the parser, so attribute name is
                // safe and unwrap() here won't ever be triggered.
                let info = CString::new(attr).unwrap().into_raw();
                MatcherErrorFfi {
                    err_type: ATTRIB_UNAVAIL,
                    info,
                    info2: ptr::null_mut(),
                }
            }

            MatcherError::InvalidRegex { regex, errmsg } => {
                // NUL bytes in filter expressions are disallowed by the parser, so regex here is safe
                // and unwrap() won't be triggered.
                let info = CString::new(regex).unwrap().into_raw();
                // Error message comes from regerror, which is a C function. Since it returns
                // a C string, it's obvious that errmsg won't contain NUL bytes. Thus, unwrap() here
                // won't ever be triggered.
                let info2 = CString::new(errmsg).unwrap().into_raw();
                MatcherErrorFfi {
                    err_type: INVALID_REGEX,
                    info,
                    info2,
                }
            }
        }
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_get_test_attr_unavail_error() -> MatcherErrorFfi {
    matcher_error_to_ffi(MatcherError::AttributeUnavailable {
        attr: String::from("test_attribute"),
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_get_test_invalid_regex_error() -> MatcherErrorFfi {
    matcher_error_to_ffi(MatcherError::InvalidRegex {
        regex: String::from("?!"),
        errmsg: String::from("inconceivable happened!"),
    })
}
