use abort_on_panic;
use libc::c_char;
use libnewsboat::fmtstrformatter::FmtStrFormatter;
use std::ffi::{CStr, CString};

#[no_mangle]
pub extern "C" fn rs_fmtstrformatter_new() -> *mut FmtStrFormatter {
    abort_on_panic(|| Box::into_raw(Box::new(FmtStrFormatter::new())))
}

#[no_mangle]
pub extern "C" fn rs_fmtstrformatter_free(fmt: *mut FmtStrFormatter) {
    abort_on_panic(|| {
        if fmt.is_null() {
            return;
        }
        unsafe {
            Box::from_raw(fmt);
        }
    })
}

#[no_mangle]
pub extern "C" fn rs_fmtstrformatter_register_fmt(
    fmt: *mut FmtStrFormatter,
    key: c_char,
    value: *const c_char,
) {
    abort_on_panic(|| {
        let fmt = unsafe {
            assert!(!fmt.is_null());
            &mut *fmt
        };
        let value = unsafe {
            assert!(!value.is_null());
            CStr::from_ptr(value)
        }
        .to_string_lossy()
        .into_owned();
        // Keys are ASCII, so it's safe to cast c_char (i8) to u8 (0-127 map to the same bits).
        // From there, it's safe to cast to Rust's char.
        let key = key as u8 as char;
        fmt.register_fmt(key as char, value);
    })
}

#[no_mangle]
pub extern "C" fn rs_fmtstrformatter_do_format(
    fmt: *mut FmtStrFormatter,
    format: *const c_char,
    width: u32,
) -> *mut c_char {
    abort_on_panic(|| {
        let fmt = unsafe {
            assert!(!fmt.is_null());
            &mut *fmt
        };
        let format = unsafe {
            assert!(!format.is_null());
            CStr::from_ptr(format)
        }
        .to_str()
        .expect("format contained invalid UTF-8");
        let result = fmt.do_format(format, width);
        CString::new(result).unwrap().into_raw()
    })
}
