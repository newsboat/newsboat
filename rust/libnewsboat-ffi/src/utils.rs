use abort_on_panic;
use libc::{c_char, c_ulong};
use libnewsboat::logger::{self, Level};
use libnewsboat::utils;
use std::ffi::{CStr, CString};
use std::path;
use std::ptr;

#[no_mangle]
pub unsafe extern "C" fn rs_replace_all(
    input: *const c_char,
    from: *const c_char,
    to: *const c_char,
) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy().into_owned();

        let rs_from = CStr::from_ptr(from);
        let rs_from = rs_from.to_str().expect("rs_from contained invalid UTF-8");

        let rs_to = CStr::from_ptr(to);
        let rs_to = rs_to.to_str().expect("rs_to contained invalid UTF-8");

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
pub unsafe extern "C" fn rs_consolidate_whitespace(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
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
pub unsafe extern "C" fn rs_resolve_tilde(path: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_path = CStr::from_ptr(path);
        // We simply assume that all the paths are in UTF-8 -- hence to_string_lossy().
        let rs_path = rs_path.to_string_lossy().into_owned();

        let result = utils::resolve_tilde(path::PathBuf::from(rs_path));
        // We simply assume that all the paths are in UTF-8 -- hence to_string_lossy().
        let result = result.to_string_lossy().into_owned();

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
pub unsafe extern "C" fn rs_to_u(in_str: *const c_char, default_value: u32) -> u32 {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(in_str);
        let rs_str = rs_str.to_string_lossy().into_owned();

        utils::to_u(rs_str, default_value)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_absolute_url(
    base_url: *const c_char,
    link: *const c_char,
) -> *mut c_char {
    abort_on_panic(|| {
        let rs_base_url = CStr::from_ptr(base_url);
        let rs_base_url = rs_base_url.to_string_lossy();
        let rs_link = CStr::from_ptr(link);
        let rs_link = rs_link.to_string_lossy();

        let ret = utils::absolute_url(&rs_base_url, &rs_link);
        CString::new(ret).unwrap().into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_is_special_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(in_str);
        let rs_str = rs_str.to_string_lossy();
        utils::is_special_url(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_is_http_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(in_str);
        let rs_str = rs_str.to_string_lossy();
        utils::is_http_url(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_is_query_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(in_str);
        let rs_str = rs_str.to_string_lossy();
        utils::is_query_url(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_is_filter_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(in_str);
        let rs_str = rs_str.to_string_lossy();
        utils::is_filter_url(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_is_exec_url(in_str: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(in_str);
        let rs_str = rs_str.to_string_lossy();
        utils::is_exec_url(&rs_str)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_censor_url(url: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_url = CStr::from_ptr(url);
        let rs_url = rs_url.to_string_lossy();
        CString::new(utils::censor_url(&rs_url)).unwrap().into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_quote_for_stfl(string: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_string = CStr::from_ptr(string);
        let rs_string = rs_string.to_string_lossy();
        CString::new(utils::quote_for_stfl(&rs_string))
            // CString::new declares returning a NulError if the passed string contains \0-bytes.
            // Unwrap is checked here, because quote_for_stfl receives a regular, c-style string,
            // and only inserts additional, non-null bytes ('>'), so unwrapping it will always
            // succeed.
            .unwrap()
            .into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_trim_end(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::trim_end(rs_input);

        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_trim(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy().into_owned();

        let result = utils::trim(rs_input);

        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_quote(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
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
pub unsafe extern "C" fn rs_quote_if_necessary(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
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
pub extern "C" fn rs_get_random_value(rs_max: u32) -> u32 {
    abort_on_panic(|| utils::get_random_value(rs_max))
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
pub unsafe extern "C" fn rs_make_title(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
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
pub unsafe extern "C" fn rs_is_valid_color(input: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy();
        utils::is_valid_color(&rs_input)
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_get_basename(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy();
        let output = utils::get_basename(&rs_input);
        let result = CString::new(output).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_gentabs(input: *const c_char) -> usize {
    abort_on_panic(|| {
        let rs_str = CStr::from_ptr(input);
        let rs_str = rs_str.to_string_lossy().into_owned();
        utils::gentabs(&rs_str)
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
pub unsafe extern "C" fn rs_is_valid_attribute(attribute: *const c_char) -> bool {
    abort_on_panic(|| {
        let rs_attribute = CStr::from_ptr(attribute);
        let rs_attribute = rs_attribute.to_string_lossy();
        utils::is_valid_attribute(&rs_attribute)
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
pub unsafe extern "C" fn rs_get_command_output(input: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let rs_input = CStr::from_ptr(input);
        let rs_input = rs_input.to_string_lossy();
        let output = utils::get_command_output(&rs_input);
        // String::from_utf8_lossy() will replace invalid unicode (including null bytes) with U+FFFD,
        // so this shouldn't be able to panic
        let result = CString::new(output).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_extract_filter(
    line: *const c_char,
    filter: *mut *mut c_char,
    url: *mut *mut c_char,
) {
    abort_on_panic(|| {
        let rs_line = CStr::from_ptr(line);
        let rs_line = rs_line.to_string_lossy();
        let mut rs_filter = String::new();
        let mut rs_url = String::new();
        utils::extract_filter(&rs_line, &mut rs_filter, &mut rs_url);
        // String::from_utf8_lossy() will replace invalid unicode (including null bytes) with U+FFFD,
        // so this shouldn't be able to panic
        let rs_filter = CString::new(rs_filter).unwrap();
        let rs_url = CString::new(rs_url).unwrap();
        *filter = rs_filter.into_raw();
        *url = rs_url.into_raw();
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_run_command(command: *const c_char, param: *const c_char) {
    abort_on_panic(|| {
        let command = CStr::from_ptr(command);
        let command = command.to_str().expect("command contained invalid UTF-8");

        let param = CStr::from_ptr(param);
        let param = param.to_str().expect("param contained invalid UTF-8");

        utils::run_command(command, param);
    })
}

#[no_mangle]
pub unsafe extern "C" fn rs_run_program(
    argv: *mut *mut c_char,
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
        let input = input.to_str().expect("input contained invalid UTF-8");

        let output = utils::run_program(&argv, input);

        // String::from_utf8_lossy() will replace invalid unicode (including null bytes) with U+FFFD,
        // so this shouldn't be able to panic
        let result = CString::new(output).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_program_version() -> *mut c_char {
    abort_on_panic(|| {
        let version = utils::program_version();
        // `utils::program_version` generates the string either from a textual representation of
        // current Git commit id (which has no internal nul bytes), or a combination of major,
        // minor, and patch versions of `libnewsboat` crate (which, too, has no internal nul
        // bytes). Therefore there is no reason from `new()` to return `None`.
        let result = CString::new(version).unwrap();
        result.into_raw()
    })
}

#[no_mangle]
pub extern "C" fn rs_newsboat_version_major() -> u32 {
    abort_on_panic(utils::newsboat_major_version)
}

#[no_mangle]
pub unsafe extern "C" fn rs_strip_comments(line: *const c_char) -> *mut c_char {
    abort_on_panic(|| {
        let line = CStr::from_ptr(line);
        let line = line.to_str().expect("line contained invalid UTF-8");

        let result = utils::strip_comments(line);

        // `result` contains a subset of `line`, which is a C string. Thus, we conclude that
        // `result` doesn't contain null bytes. Therefore, `CString::new` always returns `Some`.
        let result = CString::new(result).unwrap();
        result.into_raw()
    })
}
