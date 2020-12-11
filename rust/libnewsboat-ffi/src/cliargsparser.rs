use crate::abort_on_panic;
use libc::{c_char, c_void};
use libnewsboat::cliargsparser::CliArgsParser;
use std::ffi::{CStr, CString};
use std::mem;
use std::panic::{RefUnwindSafe, UnwindSafe};
use std::path::PathBuf;
use std::ptr;
use std::slice;

#[cxx::bridge(namespace = "newsboat::cliargsparser::bridged")]
mod bridged {
    extern "Rust" {
        type CliArgsParser;

        fn create(argv: Vec<String>) -> Box<CliArgsParser>;

        fn do_import(cliargsparser: &CliArgsParser) -> bool;
        fn do_export(cliargsparser: &CliArgsParser) -> bool;
        fn do_vacuum(cliargsparser: &CliArgsParser) -> bool;
        fn do_cleanup(cliargsparser: &CliArgsParser) -> bool;
        fn do_show_version(cliargsparser: &CliArgsParser) -> u64;
        fn silent(cliargsparser: &CliArgsParser) -> bool;
        fn using_nonstandard_configs(cliargsparser: &CliArgsParser) -> bool;
        fn should_print_usage(cliargsparser: &CliArgsParser) -> bool;
        fn refresh_on_start(cliargsparser: &CliArgsParser) -> bool;

        fn importfile(cliargsparser: &CliArgsParser) -> String;
        fn program_name(cliargsparser: &CliArgsParser) -> String;
        fn display_msg(cliargsparser: &CliArgsParser) -> String;
    }

    extern "C++" {
        // cxx uses `std::out_of_range`, but doesn't include the header that defines that
        // exception. So we do it for them.
        include!("stdexcept");
        // Also inject a header that defines ptrdiff_t. Note this is *not* a C++ header, because
        // cxx uses a non-C++ name of the type.
        include!("stddef.h");
    }
}

fn create(argv: Vec<String>) -> Box<CliArgsParser> {
    Box::new(CliArgsParser::new(argv))
}

fn do_import(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.importfile.is_some()
}

fn do_export(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.do_export
}

fn do_vacuum(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.do_vacuum
}

fn do_cleanup(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.do_cleanup
}

fn do_show_version(cliargsparser: &CliArgsParser) -> u64 {
    cliargsparser.show_version as u64
}

fn silent(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.silent
}

fn using_nonstandard_configs(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.using_nonstandard_configs()
}

fn should_print_usage(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.should_print_usage
}

fn refresh_on_start(cliargsparser: &CliArgsParser) -> bool {
    cliargsparser.refresh_on_start
}

fn importfile(cliargsparser: &CliArgsParser) -> String {
    match cliargsparser.importfile.to_owned() {
        Some(path) => path.to_string_lossy().to_string(),
        None => String::new(),
    }
}

fn program_name(cliargsparser: &CliArgsParser) -> String {
    cliargsparser.program_name.to_string()
}

fn display_msg(cliargsparser: &CliArgsParser) -> String {
    cliargsparser.display_msg.to_string()
}

#[no_mangle]
pub unsafe extern "C" fn create_rs_cliargsparser(
    argc: isize,
    argv: *mut *const c_char,
) -> *mut c_void {
    abort_on_panic(|| {
        assert!(!argv.is_null());
        assert!(argc >= 0);
        let argv = slice::from_raw_parts(argv, argc as usize);

        let args = argv
            .iter()
            .map(|s_ptr| CStr::from_ptr(*s_ptr).to_string_lossy().into_owned())
            .collect::<Vec<String>>();
        Box::into_raw(Box::new(CliArgsParser::new(args))) as *mut c_void
    })
}

#[no_mangle]
pub unsafe extern "C" fn destroy_rs_cliargsparser(object: *mut c_void) {
    abort_on_panic(|| {
        if object.is_null() {
            return;
        }
        Box::from_raw(object as *mut CliArgsParser);
    })
}

unsafe fn with_cliargsparser<F, T>(object: *mut c_void, action: F, default: T) -> T
where
    F: RefUnwindSafe + Fn(&CliArgsParser) -> T,
    T: UnwindSafe,
{
    abort_on_panic(|| {
        if object.is_null() {
            return default;
        }
        let object = Box::from_raw(object as *mut CliArgsParser);
        let result = action(&object);
        // Don't destroy the object when the function finishes
        mem::forget(object);
        result
    })
}

unsafe fn with_cliargsparser_opt_pathbuf<F>(object: *mut c_void, action: F) -> *mut c_char
where
    F: RefUnwindSafe + Fn(&CliArgsParser) -> &Option<PathBuf>,
{
    with_cliargsparser(
        object,
        |o| {
            let opt: &Option<PathBuf> = action(o);
            let opt: String = match opt.to_owned() {
                Some(path) => path.to_string_lossy().to_string(),
                None => String::new(),
            };
            CString::new(opt).unwrap().into_raw()
        },
        ptr::null_mut(),
    )
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_do_read_import(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.readinfo_import_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_readinfo_import_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.readinfo_import_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_do_read_export(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.readinfo_export_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_readinfo_export_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.readinfo_export_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_should_return(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.return_code.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_return_code(object: *mut c_void) -> isize {
    with_cliargsparser(object, |o| o.return_code.unwrap_or(0) as isize, 0)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_set_url_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.url_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_url_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.url_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_set_lock_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.lock_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_lock_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.lock_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_set_cache_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.cache_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_cache_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.cache_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_set_config_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.config_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_config_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.config_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_execute_cmds(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| !o.cmds_to_execute.is_empty(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_cmds_to_execute_count(object: *mut c_void) -> usize {
    with_cliargsparser(object, |o| o.cmds_to_execute.len(), 0)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_cmd_to_execute_n(
    object: *mut c_void,
    n: usize,
) -> *mut c_char {
    if object.is_null() {
        return ptr::null_mut();
    }
    abort_on_panic(|| {
        let object = Box::from_raw(object as *mut CliArgsParser);
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
pub unsafe extern "C" fn rs_cliargsparser_set_log_file(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.log_file.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_log_file(object: *mut c_void) -> *mut c_char {
    with_cliargsparser_opt_pathbuf(object, |o| &o.log_file)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_set_log_level(object: *mut c_void) -> bool {
    with_cliargsparser(object, |o| o.log_level.is_some(), false)
}

#[no_mangle]
pub unsafe extern "C" fn rs_cliargsparser_log_level(object: *mut c_void) -> i8 {
    with_cliargsparser(
        object,
        |o| match o.log_level {
            Some(l) => l as i8,
            None => -1 as i8,
        },
        -1,
    )
}
