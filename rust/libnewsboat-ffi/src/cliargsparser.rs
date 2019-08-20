use abort_on_panic;
use libc::{c_char, c_void};
use libnewsboat::cliargsparser::CliArgsParser;
use libnewsboat::logger::Level;
use std::ffi::{CStr, CString};
use std::mem;
use std::panic::{RefUnwindSafe, UnwindSafe};
use std::ptr;
use std::slice;

#[no_mangle]
pub extern "C" fn create_rs_cliargsparser(argc: isize, argv: *mut *const c_char) -> *mut c_void {
    abort_on_panic(|| {
        assert!(!argv.is_null());
        assert!(argc >= 0);
        let argv = unsafe { slice::from_raw_parts(argv, argc as usize) };

        let args = argv
            .iter()
            .map(|s_ptr| unsafe { CStr::from_ptr(*s_ptr).to_string_lossy().into_owned() })
            .collect::<Vec<String>>();
        Box::into_raw(Box::new(CliArgsParser::new(args))) as *mut c_void
    })
}

#[no_mangle]
pub extern "C" fn destroy_rs_cliargsparser(object: *mut c_void) {
    abort_on_panic(|| {
        if object.is_null() {
            return;
        }
        unsafe {
            Box::from_raw(object);
        }
    })
}

fn with_cliargsparser<F, T>(object: *mut c_void, action: F, default: T) -> T
where
    F: RefUnwindSafe + Fn(&CliArgsParser) -> T,
    T: UnwindSafe,
{
    abort_on_panic(|| {
        if object.is_null() {
            return default;
        }
        // TODO: use abort_on_panic here
        let object = unsafe { Box::from_raw(object as *mut CliArgsParser) };
        let result = action(&object);
        // Don't destroy the object when the function finishes
        mem::forget(object);
        result
    })
}

fn with_cliargsparser_str<F>(object: *mut c_void, action: F) -> *mut c_char
where
    F: RefUnwindSafe + Fn(&CliArgsParser) -> &str,
{
    with_cliargsparser(
        object,
        |o| CString::new(action(o).to_string()).unwrap().into_raw(),
        ptr::null_mut(),
    )
}

fn with_cliargsparser_opt_str<F>(object: *mut c_void, action: F) -> *mut c_char
where
    F: RefUnwindSafe + Fn(&CliArgsParser) -> &Option<String>,
{
    with_cliargsparser(
        object,
        |o| {
            let opt = action(o);
            // Converting &Option<String> -> String - either cloned contents of Some(), or an empty
            // string
            let opt = opt
                .as_ref()
                .map(String::clone)
                .unwrap_or_else(|| String::new());
            CString::new(opt).unwrap().into_raw()
        },
        ptr::null_mut(),
    )
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_do_import(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.importfile.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_do_export(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.do_export, false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_do_vacuum(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.do_vacuum, false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_program_name(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_str(object, |o| &o.program_name)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_importfile(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.importfile)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_do_read_import(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.readinfo_import_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_readinfo_import_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.readinfo_import_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_do_read_export(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.readinfo_export_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_readinfo_export_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.readinfo_export_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_show_version(object: *mut c_void) -> usize {
    with_cliargsparser(object, |o| o.show_version, 0)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_silent(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.silent, false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_using_nonstandard_configs(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.using_nonstandard_configs(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_should_return(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.return_code.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_return_code(object: *mut c_void) -> isize {
    with_cliargsparser(object, |o| o.return_code.unwrap_or(0) as isize, 0)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_display_msg(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_str(object, |o| &o.display_msg)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_should_print_usage(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.should_print_usage, false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_refresh_on_start(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.refresh_on_start, false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_set_url_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.url_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_url_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.url_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_set_lock_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.lock_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_lock_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.lock_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_set_cache_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.cache_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_cache_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.cache_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_set_config_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.config_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_config_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.config_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_execute_cmds(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| !o.cmds_to_execute.is_empty(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_cmds_to_execute_count(object: *mut c_void) -> usize {
    with_cliargsparser(object, |o| o.cmds_to_execute.len(), 0)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_cmd_to_execute_n(object: *mut c_void, n: usize) -> *mut c_char {
    if object.is_null() {
        return ptr::null_mut();
    }
    abort_on_panic(|| {
        let object = unsafe { Box::from_raw(object as *mut CliArgsParser) };
        let result = if n < object.cmds_to_execute.len() {
            CString::new(object.cmds_to_execute[n].clone())
                .unwrap()
                .into_raw()
        } else {
            ptr::null_mut()
        };
        // Don't destroy the object when the function finishes
        mem::forget(object);
        result
    })
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_set_log_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.log_file.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_log_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_str(object, |o| &o.log_file)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_set_log_level(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.log_level.is_some(), false)
}

#[no_mangle]
pub extern "C" fn rs_cliargsparser_log_level(object: *mut c_void) -> u8 {
    with_cliargsparser(object, |o| o.log_level.unwrap_or(Level::None) as u8, 0)
}
