extern crate libc;
extern crate libnewsboat;

use libc::c_char;
use std::ffi::{CStr, CString};
use std::panic::{UnwindSafe, catch_unwind};
use std::process::abort;
use std::ptr;

use libnewsboat::{utils, logger, human_panic};

/// Runs a Rust function, and if it panics, calls abort(); otherwise returns what function
/// returned.
fn abort_on_panic<F: FnOnce() -> R + UnwindSafe, R>(function: F) -> R {
    match catch_unwind(function) {
        Ok(result) => result,
        Err(_cause) => abort(),
    }
}

#[no_mangle]
pub extern "C" fn rs_replace_all(
    input: *const c_char,
    from: *const c_char,
    to: *const c_char)
    -> *mut c_char
{
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let rs_from = unsafe { CStr::from_ptr(from) };
        let rs_from = rs_from.to_str()
            .expect("rs_from contained invalid UTF-8");

        let rs_to = unsafe { CStr::from_ptr(to) };
        let rs_to = rs_to.to_str()
            .expect("rs_to contained invalid UTF-8");

        let result = utils::replace_all(rs_input, rs_from, rs_to);
        // Panic here can't happen because:
        // 1. panic can only happen if `result` contains null bytes;
        // 2. `result` contains what `input` contained, plus maybe what `to` contains (if there were
        //    replacements);
        // 3. neither `input` nor `to` could contain null bytes because they're null-terminated
        //    strings we got from C.
        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_consolidate_whitespace( input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::consolidate_whitespace(rs_input);
        // Panic here can't happen because:
        // 1. panic can only happen if `result` contains null bytes;
        // 2. `result` contains what `input` contained, and input is a
        // null-terminated string from C.
        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_resolve_tilde( path: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_path = unsafe { CStr::from_ptr(path) };
        let rs_path = rs_path.to_string_lossy().into_owned();

        let result = utils::resolve_tilde(rs_path);

        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_to_u(in_str: *const c_char, default_value: u32) -> u32
{
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(in_str) };
        let rs_str = rs_str.to_string_lossy().into_owned();

        utils::to_u( rs_str, default_value)
    })
}

#[no_mangle]
pub extern "C" fn rs_absolute_url(base_url: *const c_char, link: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_base_url = unsafe { CStr::from_ptr(base_url) };
        let rs_base_url = rs_base_url.to_string_lossy();
        let rs_link = unsafe { CStr::from_ptr(link) };
        let rs_link = rs_link.to_string_lossy();

        let ret = utils::absolute_url(&rs_base_url, &rs_link);
        CString::new(ret).unwrap().into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_is_special_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(in_str) };
        let rs_str = rs_str.to_string_lossy();
        utils::is_special_url(&rs_str)
    })
}

#[no_mangle]
pub extern "C" fn rs_is_http_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(in_str) };
        let rs_str = rs_str.to_string_lossy();
        utils::is_http_url(&rs_str)
    })
}

#[no_mangle]
pub extern "C" fn rs_is_query_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(in_str) };
        let rs_str = rs_str.to_string_lossy();
        utils::is_query_url(&rs_str)
    })
}

#[no_mangle]
pub extern "C" fn rs_is_filter_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(in_str) };
        let rs_str = rs_str.to_string_lossy();
        utils::is_filter_url(&rs_str)
    })
}

#[no_mangle]
pub extern "C" fn rs_is_exec_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(in_str) };
        let rs_str = rs_str.to_string_lossy();
        utils::is_exec_url(&rs_str)
    })
}

#[no_mangle]
pub extern "C" fn rs_censor_url(url: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_url = unsafe { CStr::from_ptr(url) };
        let rs_url = rs_url.to_string_lossy();
        CString::new(utils::censor_url(&rs_url)).unwrap().into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_trim_end(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::trim_end(rs_input);

        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_trim(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::trim(rs_input);

        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_quote(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let output = utils::quote(rs_input);
        // Panic here can't happen because:
        // 1. panic can only happen if `output` contains null bytes;
        // 2. `output` contains what `input` contained, plus quotes and backslases,
        // and input is a null-terminated string from C.
        let output = CString::new(output).unwrap();
        output.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_quote_if_necessary(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let output = utils::quote_if_necessary(rs_input);
        // Panic here can't happen because:
        // 1. panic can only happen if `output` contains null bytes;
        // 2. `output` contains what `input` contained, plus quotes and backslashes
        // if necessary, and input is a null-terminated string from C.
        let output = CString::new(output).unwrap();
        output.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_get_random_value(rs_max: u32 ) -> u32 {
    abort_on_panic(|| {
        utils::get_random_value(rs_max)
    })
}

#[no_mangle]
pub extern "C" fn rs_unescape_url(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::unescape_url(rs_input);
        // Panic here can't happen because:
        // 1. It would have already occured within unescape_url
        // 2. panic can only happen if `result` contains null bytes;
        // 3. `result` contains what `input` contained, and input is a
        // null-terminated string from C.
        if result.is_some(){
                let result = CString::new(result.unwrap()).unwrap();
                return result.into_raw()
        }
        return ptr::null_mut()
    })
}

#[no_mangle]
pub extern "C" fn rs_make_title(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::make_title(rs_input);
        // Panic here can't happen because:
        // 1. panic can only happen if `result` contains null bytes;
        // 2. panic due to null bytes would be dealt with in unescape_url
        // 3. `result` contains what `input` contained, and input is a
        // null-terminated string from C.
        let result = CString::new(result).unwrap();

        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_cstring_free(string: *mut c_char) {
    abort_on_panic(|| {
        unsafe {
            if string.is_null() { return }
            CString::from_raw(string);
        };
    })
}

#[no_mangle]
pub extern "C" fn rs_get_default_browser() -> *mut c_char {
    abort_on_panic(|| {
        let browser = utils::get_default_browser();
        // Panic here can't happen because:
        //1. panic can only happen if `result` contains null bytes; 
        //2. std::env::var returns an error if the variable contains invalid Unicode, 
        // In that case get_default_browser will return "lynx", which obviously doesn't contain null bytes.
        let result = CString::new(browser).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_is_valid_color(input: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy();
        utils::is_valid_color(&rs_input)
    })
}

#[no_mangle]
pub extern "C" fn rs_is_valid_attribute(attribute: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_attribute = unsafe { CStr::from_ptr(attribute) };
        let rs_attribute = rs_attribute.to_string_lossy();
        utils::is_valid_attribute(&rs_attribute)
    })
}

#[no_mangle]
pub extern "C" fn rs_strwidth(input: *const c_char) -> usize{
    abort_on_panic(|| {
        let rs_str = unsafe { CStr::from_ptr(input) };
        let rs_str = rs_str.to_string_lossy().into_owned();
        utils::strwidth(&rs_str)
    })
}

#[no_mangle]
pub extern "C" fn rs_is_valid_podcast_type(mimetype: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_mimetype = unsafe { CStr::from_ptr(mimetype) };
        let rs_mimetype = rs_mimetype.to_string_lossy();
        utils::is_valid_podcast_type(&rs_mimetype)
    })
}

#[no_mangle]
pub extern "C" fn rs_log(level: logger::Level, message: *const c_char) {
    abort_on_panic(|| {
        let message = unsafe { CStr::from_ptr(message) };
        logger::get_instance().log_raw(level, message.to_bytes());
    })
}

#[no_mangle]
pub extern "C" fn rs_set_loglevel(level: logger::Level) {
    abort_on_panic(|| {
        logger::get_instance().set_loglevel(level);
    })
}

#[no_mangle]
pub extern "C" fn rs_set_logfile(logfile: *const c_char) {
    abort_on_panic(|| {
        let logfile = unsafe { CStr::from_ptr(logfile) };
        let logfile = logfile.to_str().expect("logfile path contained invalid UTF-8");
        logger::get_instance().set_logfile(logfile);
    })
}

#[no_mangle]
pub extern "C" fn rs_set_user_error_logfile(user_error_logfile: *const c_char) {
    abort_on_panic(|| {
        let user_error_logfile = unsafe { CStr::from_ptr(user_error_logfile) };
        let user_error_logfile = user_error_logfile.to_str().expect("user_error_logfile path contained invalid UTF-8");
        logger::get_instance().set_user_error_logfile(user_error_logfile);
    })
}

#[no_mangle]
pub extern "C" fn rs_get_loglevel() -> u64 {
    abort_on_panic(|| {
        logger::get_instance().get_loglevel() as u64
    })
}

#[no_mangle]
pub extern "C" fn rs_setup_human_panic() {
    abort_on_panic(|| {
        human_panic::setup();
    })
}

#[no_mangle]
pub extern "C" fn rs_get_command_output(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = unsafe { CStr::from_ptr(input) };
        let rs_input = rs_input.to_string_lossy();
        let output = utils::get_command_output(&rs_input);
        // String::from_utf8_lossy() will replace invalid unicode (including null bytes) with U+FFFD,
        // so this shouldn't be able to panic
        let result = CString::new(output).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_run_command(command: *const c_char, param: *const c_char) {
    abort_on_panic(|| {
        let command = unsafe { CStr::from_ptr(command) };
        let command = command.to_str()
            .expect("command contained invalid UTF-8");

        let param = unsafe { CStr::from_ptr(param) };
        let param = param.to_str()
            .expect("param contained invalid UTF-8");

        utils::run_command(command, param);
    })
}

#[no_mangle]
pub extern "C" fn rs_run_program(argv: *mut *mut c_char, input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let argv = unsafe {
            let mut result: Vec<&str> = Vec::new();
            let mut cur_ptr = argv;

            let mut offset: usize = 0;
            while !(*cur_ptr).is_null() {
                let arg = CStr::from_ptr(*cur_ptr);
                let arg = arg.to_str()
                    .expect(&format!("argument at offset {} contained invalid UTF-8", offset));

                result.push(arg);

                offset += 1;
                cur_ptr = cur_ptr.offset(1isize);
            }

            result
        };


        let input = unsafe { CStr::from_ptr(input) };
        let input = input.to_str()
            .expect("input contained invalid UTF-8");

        let output = utils::run_program(&argv, input);

        // String::from_utf8_lossy() will replace invalid unicode (including null bytes) with U+FFFD,
        // so this shouldn't be able to panic
        let result = CString::new(output).unwrap();
        result.into_raw()
    })
}
