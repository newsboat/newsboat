use crate::abort_on_panic;
use libc::c_char;
use libnewsboat::matchererror::MatcherError;
use std::ffi::CString;
use std::ptr;

#[cxx::bridge(namespace = "newsboat::matchererror::bridged")]
mod bridged {
    #[repr(u8)]
    enum Type {
        AttributeUnavailable = 0,
        InvalidRegex = 1,
    }
}

#[repr(C)]
pub struct MatcherErrorFfi {
    err_type: u8, // One of the constants defined above
    info: *mut c_char,
    info2: *mut c_char,
}

#[unsafe(no_mangle)]
pub fn matcher_error_to_ffi(error: MatcherError) -> MatcherErrorFfi {
    abort_on_panic(|| {
        match error {
            MatcherError::AttributeUnavailable { attr } => {
                // NUL bytes in filter expressions are disallowed by the parser, so attribute name is
                // safe and unwrap() here won't ever be triggered.
                let info = CString::new(attr).unwrap().into_raw();
                MatcherErrorFfi {
                    err_type: bridged::Type::AttributeUnavailable.repr,
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
                    err_type: bridged::Type::InvalidRegex.repr,
                    info,
                    info2,
                }
            }
        }
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rs_get_test_attr_unavail_error() -> MatcherErrorFfi {
    matcher_error_to_ffi(MatcherError::AttributeUnavailable {
        attr: String::from("test_attribute"),
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rs_get_test_invalid_regex_error() -> MatcherErrorFfi {
    matcher_error_to_ffi(MatcherError::InvalidRegex {
        regex: String::from("?!"),
        errmsg: String::from("inconceivable happened!"),
    })
}
