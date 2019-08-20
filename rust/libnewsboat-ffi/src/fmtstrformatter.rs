use abort_on_panic;
use libc::{c_char, c_void};
use libnewsboat::fmtstrformatter::FmtStrFormatter;
use std::ffi::{CStr, CString};
use std::mem;

#[no_mangle]
pub extern "C" fn rs_fmtstrformatter_new() -> *mut c_void {
    abort_on_panic(|| Box::into_raw(Box::new(FmtStrFormatter::new())) as *mut c_void)
}

#[no_mangle]
pub unsafe extern "C" fn rs_fmtstrformatter_free(fmt: *mut c_void) {
    abort_on_panic(|| {
        if fmt.is_null() {
            return;
        }
        Box::from_raw(fmt as *mut FmtStrFormatter);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_fmtstrformatter_register_fmt(
    fmt: *mut c_void,
    key: c_char,
    value: *const c_char,
) {
    abort_on_panic(|| {
        let mut fmt = {
            assert!(!fmt.is_null());
            Box::from_raw(fmt as *mut FmtStrFormatter)
        };
        let value = {
            assert!(!value.is_null());
            CStr::from_ptr(value)
        }
        .to_string_lossy()
        .into_owned();
        // Keys are ASCII, so it's safe to cast c_char (i8) to u8 (0-127 map to the same bits).
        // From there, it's safe to cast to Rust's char.
        let key = key as u8 as char;
        fmt.register_fmt(key as char, value);

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(fmt);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_fmtstrformatter_do_format(
    fmt: *mut c_void,
    format: *const c_char,
    width: u32,
) -> *mut c_char {
    abort_on_panic(|| {
        let fmt = {
            assert!(!fmt.is_null());
            Box::from_raw(fmt as *mut FmtStrFormatter)
        };
        let format = {
            assert!(!format.is_null());
            CStr::from_ptr(format)
        }
        .to_str()
        .expect("format contained invalid UTF-8");
        let result = fmt.do_format(format, width);
        let result = CString::new(result).unwrap().into_raw();

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(fmt);

        result
    })
}
