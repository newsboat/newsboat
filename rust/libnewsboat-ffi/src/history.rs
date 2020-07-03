use crate::abort_on_panic;
use libc::{c_char, c_void};
use libnewsboat::history::History;
use std::ffi::{CStr, CString};
use std::mem;

#[no_mangle]
pub extern "C" fn rs_history_new() -> *mut c_void {
    abort_on_panic(|| Box::into_raw(Box::new(History::new())) as *mut c_void)
}

#[no_mangle]
pub unsafe extern "C" fn rs_history_free(hst: *mut c_void) {
    abort_on_panic(|| {
        if hst.is_null() {
            return;
        }
        Box::from_raw(hst as *mut History);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_history_add_line(hst: *mut c_void, line: *const c_char) {
    abort_on_panic(|| {
        let mut hst = {
            assert!(!hst.is_null());
            Box::from_raw(hst as *mut History)
        };
        let line = {
            assert!(!line.is_null());
            CStr::from_ptr(line)
        }
        .to_string_lossy()
        .into_owned();

        hst.add_line(line);

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(hst);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_history_previous_line(hst: *mut c_void) -> *mut c_char {
    abort_on_panic(|| {
        let mut hst = {
            assert!(!hst.is_null());
            Box::from_raw(hst as *mut History)
        };
        let result = hst.previous_line();
        // Panic here can't happen because:
        // 1. panic can only happen if `result` contains null bytes;
        // 2. `result` either contains what `line` in `rs_history_add_line` contained
        // and `line` is a null-terminated string from c
        // 3. or `result` is an empty string so we are certain that `result`
        // doesn't have null bytes in the middle.
        let result = CString::new(result).unwrap().into_raw();

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(hst);

        result
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_history_next_line(hst: *mut c_void) -> *mut c_char {
    abort_on_panic(|| {
        let mut hst = {
            assert!(!hst.is_null());
            Box::from_raw(hst as *mut History)
        };
        let result = hst.next_line();
        // Panic here can't happen because:
        // 1. panic can only happen if `result` contains null bytes;
        // 2. `result` either contains what `line` in `rs_history_add_line` contained
        // and `line` is a null-terminated string from c
        // 3. or `result` is an empty string so we are certain that `result`
        // doesn't have null bytes in the middle.
        let result = CString::new(result).unwrap().into_raw();

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(hst);

        result
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_history_load_from_file(hst: *mut c_void, file: *const c_char) {
    abort_on_panic(|| {
        let mut hst = {
            assert!(!hst.is_null());
            Box::from_raw(hst as *mut History)
        };
        let file = {
            assert!(!file.is_null());
            CStr::from_ptr(file)
        }
        .to_string_lossy()
        .into_owned();

        // Original C++ code did nothing if it failed to open file.
        // Returns a [must_use] type so safely ignoring the value.
        let _ = hst.load_from_file(file);

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(hst);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_history_save_to_file(
    hst: *mut c_void,
    file: *const c_char,
    limit: usize,
) {
    abort_on_panic(|| {
        let hst = {
            assert!(!hst.is_null());
            Box::from_raw(hst as *mut History)
        };
        let file = {
            assert!(!file.is_null());
            CStr::from_ptr(file)
        }
        .to_string_lossy()
        .into_owned();

        // Original C++ code did nothing if it failed to open file.
        // Returns a [must_use] type so safely ignoring the value.
        let _ = hst.save_to_file(file, limit);

        // Do not deallocate the object - C still has a pointer to it
        mem::forget(hst);
    })
}
