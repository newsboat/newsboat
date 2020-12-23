use crate::abort_on_panic;
use libc::c_char;
use libnewsboat::utils::{self, *};
use std::ffi::{CStr, CString};
use std::path::{Path, PathBuf};

#[cxx::bridge(namespace = "newsboat::utils")]
mod ffi {
    struct FilterUrlParts {
        script_name: String,
        url: String,
    }

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
        fn strnaturalcmp(a: &str, b: &str) -> isize;
        fn strwidth(rs_str: &str) -> usize;
        fn strwidth_stfl(rs_str: &str) -> usize;
        fn extract_filter(line: &str) -> FilterUrlParts;
        fn newsboat_major_version() -> u32;

        // This function is wrapped on the Rust side to convert its output type from
        // a platform-specific `c_ulong`/`unsigned long` to platform-agnostic `u64`. Since we don't
        // need a wrapped on the C++ side, we chose to put this function in this module rather than
        // the "bridged" one.
        fn get_auth_method(method: &str) -> u64;
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

        fn resolve_tilde(path: &str) -> String;
        fn resolve_relative(reference: &str, path: &str) -> String;
        fn getcwd() -> String;
        fn mkdir_parents(path: &str, mode: u32) -> isize;

        fn unescape_url(url: String, success: &mut bool) -> String;
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

fn get_auth_method(method: &str) -> u64 {
    utils::get_auth_method(method) as u64
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

fn read_text_file(
    filename: String,
    contents: &mut Vec<String>,
    error_line_number: &mut u64,
    error_reason: &mut String,
) -> bool {
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

fn strnaturalcmp(a: &str, b: &str) -> isize {
    use std::cmp::Ordering;
    match utils::strnaturalcmp(a, b) {
        Ordering::Less => -1,
        Ordering::Equal => 0,
        Ordering::Greater => 1,
    }
}

fn extract_filter(line: &str) -> ffi::FilterUrlParts {
    let result = utils::extract_filter(line);
    ffi::FilterUrlParts {
        script_name: result.script_name,
        url: result.url,
    }
}

fn resolve_tilde(path: &str) -> String {
    let path = PathBuf::from(path);
    let result = utils::resolve_tilde(path);
    result.to_string_lossy().to_string()
}

fn resolve_relative(reference: &str, path: &str) -> String {
    let reference = Path::new(reference);
    let path = Path::new(path);
    let result = utils::resolve_relative(&reference, &path);
    result.to_string_lossy().to_string()
}

fn getcwd() -> String {
    utils::getcwd()
        .map(|path| path.to_string_lossy().to_string())
        .unwrap_or_else(|_| String::new())
}

fn mkdir_parents(path: &str, mode: u32) -> isize {
    let path = Path::new(path);
    match utils::mkdir_parents(&path, mode) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

fn unescape_url(url: String, success: &mut bool) -> String {
    match utils::unescape_url(url) {
        Some(result) => {
            *success = true;
            result
        }
        None => {
            *success = false;
            String::new()
        }
    }
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
pub unsafe extern "C" fn rs_get_string(line: *const c_char) -> *mut c_char {
    let line = CStr::from_ptr(line);
    let result = CString::from(line);
    result.into_raw()
}
