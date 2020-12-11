use crate::abort_on_panic;
use libc::{c_char, c_ulong};
use libnewsboat::utils::{self, *};
use libnewsboat::{
    log,
    logger::{self, Level},
};
use std::ffi::{CStr, CString};
use std::path;
use std::ptr;

#[cxx::bridge(namespace = "newsboat::utils")]
mod ffi {
    extern "Rust" {
        fn get_random_value(max: u32) -> u32;
        fn is_special_url(url: &str) -> bool;
        fn is_http_url(url: &str) -> bool;
        fn is_query_url(url: &str) -> bool;
        fn is_filter_url(url: &str) -> bool;
        fn is_exec_url(url: &str) -> bool;
        fn is_valid_color(color: &str) -> bool;
        fn is_valid_attribute(attribute: &str) -> bool;
        fn gentabs(string: &str) -> usize;
        fn run_command(cmd: &str, param: &str);
    }
}

// Functions that should be wrapped on the C++ side for ease of use.
#[cxx::bridge(namespace = "newsboat::utils::bridged")]
mod bridged {
    extern "Rust" {
        fn to_u(input: String, default_value: u32) -> u32;

        fn run_non_interactively(command: &str, caller: &str, exit_code: &mut u8) -> bool;

        fn read_text_file(
            filename: String,
            contents: &mut Vec<String>,
            error_line_number: &mut u64,
            error_reason: &mut String,
        ) -> bool;

        fn replace_all(input: String, from: &str, to: &str) -> String;
        fn consolidate_whitespace(input: String) -> String;
        fn absolute_url(base_url: &str, link: &str) -> String;
        fn censor_url(url: &str) -> String;
        fn quote_for_stfl(string: &str) -> String;
        fn trim(rs_str: String) -> String;
        fn trim_end(rs_str: String) -> String;
        fn quote(input: String) -> String;
        fn quote_if_necessary(input: String) -> String;
        fn make_title(rs_str: String) -> String;
        fn get_default_browser() -> String;
        fn substr_with_width(string: &str, max_width: usize) -> String;
        fn substr_with_width_stfl(string: &str, max_width: usize) -> String;
        fn get_command_output(cmd: &str) -> String;
        fn get_basename(input: &str) -> String;
        fn program_version() -> String;
        fn strip_comments(line: &str) -> &str;
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

fn run_non_interactively(command: &str, caller: &str, exit_code: &mut u8) -> bool {
    match utils::run_non_interactively(command, caller) {
        Some(e) => {
            *exit_code = e;
            true
        }
        None => false,
    }
}

pub fn read_text_file(
    filename: String,
    contents: &mut Vec<String>,
    error_line_number: &mut u64,
    error_reason: &mut String,
) -> bool {
    use std::path::Path;

    match utils::read_text_file(Path::new(&filename)) {
        Ok(c) => {
            *contents = c;
            true
        }
        Err(e) => {
            use utils::ReadTextFileError::*;
            match e {
                CantOpen { reason } => {
                    *error_line_number = 0;
                    *error_reason = reason.to_string();
                }

                LineError {
                    line_number,
                    reason,
                } => {
                    *error_line_number = line_number as u64;
                    *error_reason = reason.to_string();
                }
            }
            false
        }
    }
}

#[repr(C)]
pub struct FilterUrl {
    filter: *mut c_char,
    url: *mut c_char,
}

#[no_mangle]
pub unsafe extern "C" fn rs_resolve_tilde(path: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_path = CStr::from_ptr(path);
        // We simply assume that all the paths are in UTF-8 -- hence to_string_lossy().
        let rs_path = rs_path.to_string_lossy().into_owned();

        let result = utils::resolve_tilde(path::PathBuf::from(rs_path));
        // We simply assume that all the paths are in UTF-8 -- hence to_string_lossy().
        let result = result.to_string_lossy().into_owned();

        // `result` consists of:
        // 1. a path to the home dir, which can't contain NUL bytes, since it has to be handled by
        //    C APIs;
        // 2. a path delimiter, which is a slash and not a NUL byte;
        // 3. the original input, `path`, which came here as a C string, so doesn't contain NUL
        //    bytes.
        //
        // Thus, `unwrap` won't panic.
        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_resolve_relative(
    reference: *const c_char,
    path: *const c_char,
) -> *mut c_char {
    use std::path::Path;
    abort_on_panic(|| {
        let rs_reference = CStr::from_ptr(reference);
        let rs_reference = rs_reference.to_string_lossy().into_owned();

        let rs_path = CStr::from_ptr(path);
        let rs_path = rs_path.to_string_lossy().into_owned();

        let result = utils::resolve_relative(Path::new(&rs_reference), Path::new(&rs_path));

        // result.to_str().unwrap()' won't panic because it is either
        // - rs_path
        // - combination of reference and rs_path
        //   which are both valid strings
        //
        // CString::new(...).unwrap() won't panic for the above reasons, the strings that went into
        // it are valid strings
        let result = CString::new(result.to_str().unwrap()).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_get_auth_method(input: *const c_char) -> c_ulong {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy().into_owned();

        utils::get_auth_method(&rs_input)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_unescape_url(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::unescape_url(rs_input);
        if let Some(result) = result {
            // Panic here can't happen because:
            // 1. It would have already occured within unescape_url
            // 2. panic can only happen if `result` contains null bytes;
            // 3. `result` contains what `input` contained, and input is a
            // null-terminated string from C.
            let result = CString::new(result).unwrap();
            return result.into_raw();
        }
        ptr::null_mut()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_run_interactively(
    command: *const c_char,
    caller: *const c_char,
    success: *mut bool,
) -> u8 {
    abort_on_panic(|| {
        let command = CStr::from_ptr(command);
        // This won't panic because all strings in Newsboat are in UTF-8
        let command = command.to_str().expect("command contained invalid UTF-8");

        let caller = CStr::from_ptr(caller);
        // This won't panic because all strings in Newsboat are in UTF-8
        let caller = caller.to_str().expect("caller contained invalid UTF-8");

        match utils::run_interactively(&command, &caller) {
            Some(exit_code) => {
                *success = true;
                exit_code
            }

            None => {
                *success = false;
                0
            }
        }
    })
}

#[no_mangle]
/// Gets the current working directory or an empty string on error.
pub extern "C" fn rs_getcwd() -> *mut c_char {
    use std::os::unix::ffi::OsStringExt;
    abort_on_panic(|| {
        let result = utils::getcwd().unwrap_or_else(|err| {
            log!(Level::Warn, "Error getting current directory: {}", err);
            path::PathBuf::new()
        });
        // Panic here can't happen because:
        // 1. panic can only happen if `result` contains null bytes;
        // 2. `result` contains the current working directory which can't contain null.
        let result = CString::new(result.into_os_string().into_vec()).unwrap();

        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_strnaturalcmp(a: *const c_char, b: *const c_char) -> isize {
    use std::cmp::Ordering;
    abort_on_panic(|| {
        let a = CStr::from_ptr(a).to_string_lossy();
        let b = CStr::from_ptr(b).to_string_lossy();
        match utils::strnaturalcmp(&a, &b) {
            Ordering::Less => -1,
            Ordering::Equal => 0,
            Ordering::Greater => 1,
        }
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_mkdir_parents(path: *const c_char, mode: u32) -> isize {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(path);
        let rs_input = rs_input.to_string_lossy().into_owned();
        match utils::mkdir_parents(&rs_input, mode) {
            Ok(()) => 0,
            Err(_) => -1,
        }
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_strwidth(input: *const c_char) -> usize {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(input);
        let rs_str = rs_str.to_string_lossy().into_owned();
        utils::strwidth(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_strwidth_stfl(input: *const c_char) -> usize {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(input);
        let rs_str = rs_str.to_string_lossy().into_owned();
        utils::strwidth_stfl(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_remove_soft_hyphens(text: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_text = CStr::from_ptr(text);
        let mut rs_text = rs_text.to_string_lossy().into_owned();
        utils::remove_soft_hyphens(&mut rs_text);
        // Panic can't happen here because:
        // It could only happen if `rs_text` contains null bytes.
        // But since it is based on a C string it can't contain null bytes
        let result = CString::new(rs_text).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_is_valid_podcast_type(mimetype: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_mimetype = CStr::from_ptr(mimetype);
        let rs_mimetype = rs_mimetype.to_string_lossy();
        utils::is_valid_podcast_type(&rs_mimetype)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_podcast_mime_to_link_type(
    mimetype: *const c_char,
    success: *mut bool,
) -> i64 {
    abort_on_panic(|| {
        let rs_mimetype = CStr::from_ptr(mimetype);
        let rs_mimetype = rs_mimetype.to_string_lossy();
        match utils::podcast_mime_to_link_type(&rs_mimetype) {
            Some(link_type) => {
                *success = true;
                link_type as i64
            }

            None => {
                *success = false;
                0 // arbitrary value -- it won't be used on the other side
            }
        }
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_run_program(
    argv: *const *mut c_char,
    input: *const c_char,
) -> *mut c_char {
    abort_on_panic(|| {
        let argv = {
            let mut result: Vec<&str> = Vec::new();
            let mut cur_ptr = argv;

            let mut offset: usize = 0;
            while !(*cur_ptr).is_null() {
                let arg = CStr::from_ptr(*cur_ptr);
                let arg = arg.to_str().unwrap_or_else(|_| {
                    panic!("argument at offset {} contained invalid UTF-8", offset)
                });

                result.push(arg);

                offset += 1;
                cur_ptr = cur_ptr.offset(1isize);
            }

            result
        };

        let input = CStr::from_ptr(input);
        // This won't panic because all strings in Newsboat are in UTF-8
        let input = input.to_str().expect("input contained invalid UTF-8");

        let output = utils::run_program(&argv, input);

        // String::from_utf8_lossy() will replace invalid unicode (including null bytes) with U+FFFD,
        // so this shouldn't be able to panic
        let result = CString::new(output).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_newsboat_version_major() -> u32 {
    abort_on_panic(utils::newsboat_major_version)
}

#[no_mangle]
pub unsafe extern "C" fn rs_extract_filter(line: *const c_char) -> FilterUrl {
    abort_on_panic(|| {
        let line = CStr::from_ptr(line);
        // This won't panic because all strings in Newsboat are in UTF-8
        let line = line.to_str().expect("line contained invalid UTF-8");

        let (filter, url) = utils::extract_filter(line);
        // `rfilter` contains a subset of `line`, which is a C string. Thus, we conclude that
        // `rfilter` doesn't contain null bytes. Therefore, `CString::new` always returns `Some`.
        let filter = CString::new(filter).unwrap();
        // `rurl` contains a subset of `line`, which is a C string. Thus, we conclude that
        // `rurl` doesn't contain null bytes. Therefore, `CString::new` always returns `Some`.
        let url = CString::new(url).unwrap();

        FilterUrl {
            filter: filter.into_raw() as *mut c_char,
            url: url.into_raw() as *mut c_char,
        }
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_get_string(line: *const c_char) -> *mut c_char {
    let line = CStr::from_ptr(line);
    let result = CString::from(line);
    result.into_raw()
}
