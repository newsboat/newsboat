use crate::abort_on_panic;
use libc::{c_char, c_void};
use libnewsboat::cliargsparser::CliArgsParser;
use libnewsboat::configpaths::ConfigPaths;
use std::ffi::{CStr, CString};
use std::mem;
use std::panic::{RefUnwindSafe, UnwindSafe};
use std::path::Path;
use std::ptr;

#[no_mangle]
pub extern "C" fn create_rs_configpaths() -> *mut c_void {
    abort_on_panic(|| Box::into_raw(Box::new(ConfigPaths::new())) as *mut c_void)
}

#[no_mangle]
pub unsafe extern "C" fn destroy_rs_configpaths(object: *mut c_void) {
    abort_on_panic(|| {
        if object.is_null() {
            return;
        };
        Box::from_raw(object as *mut ConfigPaths);
    })
}

unsafe fn with_configpaths<F, T>(object: *mut c_void, action: F, default: T) -> T
where
    F: RefUnwindSafe + Fn(&mut ConfigPaths) -> T,
    T: UnwindSafe,
{
    abort_on_panic(|| {
        if object.is_null() {
            return default;
        }

        let mut object = Box::from_raw(object as *mut ConfigPaths);
        let result = action(&mut object);

        // Don't destroy the object when the function finishes
        mem::forget(object);

        result
    })
}

unsafe fn with_configpaths_string<F>(object: *mut c_void, action: F) -> *mut c_char
where
    F: RefUnwindSafe + Fn(&mut ConfigPaths) -> String,
{
    with_configpaths(
        object,
        |o| CString::new(action(o)).unwrap().into_raw(),
        ptr::null_mut(),
    )
}

unsafe fn with_configpaths_path<F>(object: *mut c_void, action: F) -> *mut c_char
where
    F: RefUnwindSafe + Fn(&mut ConfigPaths) -> &Path,
{
    with_configpaths_string(object, |o| action(o).to_string_lossy().into_owned())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_initialized(object: *mut c_void) -> bool {
    with_configpaths(object, |o| o.initialized(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_error_message(object: *mut c_void) -> *mut c_char {
    with_configpaths_string(object, |o| o.error_message().to_owned())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_process_args(
    object: *mut c_void,
    rs_cliargsparser: *mut c_void,
) {
    abort_on_panic(|| {
        if rs_cliargsparser.is_null() {
            return;
        }

        let cliargsparser = Box::from_raw(rs_cliargsparser as *mut CliArgsParser);

        with_configpaths(object, |o| o.process_args(&cliargsparser), ());

        // Don't destroy the object when the function finishes
        mem::forget(cliargsparser);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_try_migrate_from_newsbeuter(object: *mut c_void) -> bool {
    with_configpaths(object, |o| o.try_migrate_from_newsbeuter(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_create_dirs(object: *mut c_void) -> bool {
    with_configpaths(object, |o| o.create_dirs(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_url_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.url_file())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_cache_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.cache_file())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_set_cache_file(
    object: *mut c_void,
    cstr_path: *const c_char,
) {
    abort_on_panic(|| {
        let path = {
            assert!(!cstr_path.is_null());
            CStr::from_ptr(cstr_path)
        }
        .to_string_lossy()
        .into_owned();
        with_configpaths(
            object,
            |o| o.set_cache_file(Path::new(&path).to_owned()),
            (),
        )
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_config_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.config_file())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_lock_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.lock_file())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_queue_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.queue_file())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_search_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.search_file())
}

#[no_mangle]
pub unsafe extern "C" fn rs_configpaths_cmdline_file(object: *mut c_void) -> *mut c_char {
    with_configpaths_path(object, |o| o.cmdline_file())
}
