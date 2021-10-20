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

        fn run_interactively(command: &str, caller: &str, exit_code: &mut u8) -> bool;
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
        fn md5hash(input: &str) -> String;
        fn substr_with_width(string: &str, max_width: usize) -> String;
        fn substr_with_width_stfl(string: &str, max_width: usize) -> String;
        fn get_command_output(cmd: &str) -> String;
        fn get_basename(input: &str) -> String;
        fn program_version() -> String;
        fn strip_comments(line: &str) -> &str;
        fn extract_token_quoted(line: &mut String, delimiters: &str, token: &mut String) -> bool;
        fn tokenize_quoted(line: &str, delimiters: &str) -> Vec<String>;
        fn is_valid_podcast_type(mimetype: &str) -> bool;

        fn resolve_tilde(path: &str) -> String;
        fn resolve_relative(reference: &str, path: &str) -> String;
        fn getcwd() -> String;
        fn mkdir_parents(path: &str, mode: u32) -> isize;

        fn unescape_url(url: String, success: &mut bool) -> String;

        fn remove_soft_hyphens(text: &mut String);

        fn podcast_mime_to_link_type(mime_type: &str, result: &mut i64) -> bool;

        fn run_program(argv: &[&str], input: &str) -> String;

        fn translit(tocode: &str, fromcode: &str) -> String;
        fn utf8_to_locale(text: &str) -> Vec<u8>;
        fn locale_to_utf8(text: &[u8]) -> String;
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

fn run_interactively(command: &str, caller: &str, exit_code: &mut u8) -> bool {
    match utils::run_interactively(command, caller) {
        Some(e) => {
            *exit_code = e;
            true
        }
        None => false,
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

fn extract_token_quoted(line: &mut String, delimiters: &str, token: &mut String) -> bool {
    let (token_opt, remainder) = utils::extract_token_quoted(line, delimiters);
    *line = remainder.to_owned();
    match token_opt {
        Some(t) => {
            *token = t;
            true
        }
        None => false,
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
    let result = utils::resolve_relative(reference, path);
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

fn podcast_mime_to_link_type(mime_type: &str, result: &mut i64) -> bool {
    match utils::podcast_mime_to_link_type(mime_type) {
        Some(link_type) => {
            *result = link_type as i64;
            true
        }

        None => {
            // We don't assign anything to `result` because it won't be used on the other side.
            false
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn rs_get_string(line: *const c_char) -> *mut c_char {
    let line = CStr::from_ptr(line);
    let result = CString::from(line);
    result.into_raw()
}
