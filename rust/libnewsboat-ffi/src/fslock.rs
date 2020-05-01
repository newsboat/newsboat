use crate::abort_on_panic;
use libnewsboat::fslock::FsLock;
use std::ffi::CStr;

#[no_mangle]
pub extern "C" fn rs_fslock_new() -> *mut FsLock {
    Box::into_raw(Box::new(FsLock::default()))
}

#[no_mangle]
pub extern "C" fn rs_fslock_free(ptr: *mut FsLock) {
    abort_on_panic(|| {
        if ptr.is_null() {
            return;
        }
        unsafe { Box::from_raw(ptr) };
    })
}

#[no_mangle]
pub extern "C" fn rs_fslock_try_lock(
    ptr: *mut FsLock,
    new_lock_filepath: *const libc::c_char,
    pid: *mut libc::pid_t,
) -> bool {
    abort_on_panic(|| {
        let fslock = unsafe {
            assert!(!ptr.is_null());
            &mut *ptr
        };
        let new_lock_filepath = unsafe {
            assert!(!new_lock_filepath.is_null());
            CStr::from_ptr(new_lock_filepath)
        };
        let pid = unsafe { &mut *pid };
        let new_lock_filepath = new_lock_filepath.to_string_lossy().into_owned();
        fslock.try_lock(new_lock_filepath.as_ref(), pid)
    })
}
