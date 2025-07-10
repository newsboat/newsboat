use crate::links;
use crate::logger::{self, Level};
use libc::{
    c_char, c_int, c_ulong, c_void, close, execvp, exit, fork, size_t, waitpid, E2BIG, EILSEQ,
    EINVAL,
};
use md5;
use percent_encoding::*;
use std::ffi::CString;
use std::fs::DirBuilder;
use std::io::{self, Write};
use std::os::unix::fs::DirBuilderExt;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::ptr;
use unicode_segmentation::UnicodeSegmentation;
use unicode_width::{UnicodeWidthChar, UnicodeWidthStr};
use url::Url;

pub fn replace_all(input: String, from: &str, to: &str) -> String {
    input.replace(from, to)
}

pub fn consolidate_whitespace(input: String) -> String {
    let found = input.find(|c: char| !c.is_whitespace());
    let mut result = String::new();

    if let Some(found) = found {
        let (leading, rest) = input.split_at(found);
        let lastchar = input.chars().next_back().unwrap();

        result.push_str(leading);

        let iter = rest.split_whitespace();
        for elem in iter {
            result.push_str(elem);
            result.push(' ');
        }
        result.pop();
        if lastchar.is_whitespace() {
            result.push(' ');
        }
    }

    result
}

pub fn to_u(rs_str: String, default_value: u32) -> u32 {
    rs_str.parse::<u32>().unwrap_or(default_value)
}

/// Combine a base URL and a link to a new absolute URL.
/// If the base URL is malformed or joining with the link fails, link will be returned.
pub fn absolute_url(base_url: &str, link: &str) -> String {
    Url::parse(base_url)
        .and_then(|url| url.join(link))
        .as_ref()
        .map(Url::as_str)
        .unwrap_or(link)
        .to_owned()
}

/// Path to the home directory, if known. Doesn't work on Windows.
#[cfg(not(target_os = "windows"))]
pub fn home_dir() -> Option<PathBuf> {
    // This function got deprecated because it examines HOME environment variable even on Windows,
    // which is wrong. But Newsboat doesn't support Windows, so we're fine using that.
    //
    // Cf. https://github.com/rust-lang/rust/issues/28940
    #[allow(deprecated)]
    std::env::home_dir()
}

/// Replaces tilde (`~`) at the beginning of the path with the path to user's home directory.
pub fn resolve_tilde(path: PathBuf) -> PathBuf {
    if let (Some(home), Ok(suffix)) = (home_dir(), path.strip_prefix("~")) {
        return home.join(suffix);
    }

    // Either the `path` doesn't start with tilde, or we couldn't figure out the path to the
    // home directory -- either way, it's no big deal. Let's return the original string.
    path
}

pub fn resolve_relative(reference: &Path, path: &Path) -> PathBuf {
    if path.is_relative() {
        // Will only ever panic if reference is `/`, which shouldn't be the case as reference is
        // always a file path
        return reference.parent().unwrap().join(path);
    }
    path.to_path_buf()
}

pub fn is_special_url(url: &str) -> bool {
    is_query_url(url) || is_filter_url(url) || is_exec_url(url)
}

/// Check if the given URL is a http(s) URL
pub fn is_http_url(url: &str) -> bool {
    url.starts_with("https://") || url.starts_with("http://")
}

pub fn is_query_url(url: &str) -> bool {
    url.starts_with("query:")
}

pub fn is_filter_url(url: &str) -> bool {
    url.starts_with("filter:")
}

pub fn is_exec_url(url: &str) -> bool {
    url.starts_with("exec:")
}

/// Censor URLs by replacing username and password with '*'
pub fn censor_url(url: &str) -> String {
    if !url.is_empty() && !is_special_url(url) {
        Url::parse(url)
            .map(|mut url| {
                if url.username() != "" || url.password().is_some() {
                    // can not panic. If either username or password is present we can change both.
                    url.set_username("*").unwrap();
                    url.set_password(Some("*")).unwrap();
                }
                url
            })
            .as_ref()
            .map(Url::as_str)
            .unwrap_or(url)
            .to_owned()
    } else {
        url.into()
    }
}

/// Quote a string for use with stfl by replacing all occurences of "<" with "<>"
pub fn quote_for_stfl(string: &str) -> String {
    string.replace('<', "<>")
}

/// Get basename from a URL if available else return an empty string
pub fn get_basename(input: &str) -> String {
    match Url::parse(input) {
        Ok(url) => match url.path_segments() {
            Some(mut segments) => segments.next_back().unwrap().to_string(),
            None => String::from(""),
        },
        Err(_) => String::from(""),
    }
}

pub fn get_default_browser() -> String {
    use std::env;
    env::var("BROWSER").unwrap_or_else(|_| "lynx".to_string())
}

pub fn md5hash(input: &str) -> String {
    let digest = md5::compute(input);
    let hash = format!("{digest:x}");
    hash
}

pub fn trim(rs_str: String) -> String {
    rs_str.trim().to_string()
}

pub fn trim_end(rs_str: String) -> String {
    let x: &[_] = &['\n', '\r'];
    rs_str.trim_end_matches(x).to_string()
}

pub fn quote(input: String) -> String {
    let mut input = input.replace('\"', "\\\"");
    input.insert(0, '"');
    input.push('"');
    input
}

pub fn quote_if_necessary(input: String) -> String {
    match input.find(' ') {
        Some(_) => quote(input),
        None => input,
    }
}

pub fn get_random_value(max: u32) -> u32 {
    fastrand::u32(..) % max
}

pub fn is_valid_color(color: &str) -> bool {
    const COLORS: [&str; 9] = [
        "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "default",
    ];

    if COLORS.contains(&color) {
        true
    } else if color.starts_with("color0") {
        color == "color0"
    } else if let Some(num_part) = color.strip_prefix("color") {
        num_part.parse::<u8>().is_ok()
    } else {
        false
    }
}

pub fn is_valid_attribute(attribute: &str) -> bool {
    const VALID_ATTRIBUTES: [&str; 9] = [
        "standout",
        "underline",
        "reverse",
        "blink",
        "dim",
        "bold",
        "protect",
        "invis",
        "default",
    ];
    VALID_ATTRIBUTES.contains(&attribute)
}

pub fn strwidth(rs_str: &str) -> usize {
    UnicodeWidthStr::width(rs_str)
}

/// Returns the width of `rs_str` when displayed on screen.
///
/// STFL tags (e.g. `<b>`, `<foobar>`, `</>`) are counted as having 0 width.
/// Escaped less-than sign (`<` escaped as `<>`) is counted as having a width of 1 character.
pub fn strwidth_stfl(mut s: &str) -> usize {
    let mut width = 0;
    loop {
        if let Some(pos) = s.find('<') {
            width += strwidth(&s[..pos]);
            s = &s[pos..];
            if let Some(endpos) = s.find('>') {
                if endpos == 1 {
                    // Found "<>" which stfl uses to encode a literal '<'
                    width += strwidth("<");
                }
                s = &s[endpos + 1..];
            } else {
                // '<' without closing '>' so ignore rest of string
                break;
            }
        } else {
            width += strwidth(s);
            break;
        }
    }

    width
}

/// Returns a longest substring fits to the given width.
/// Returns an empty string if `str` is an empty string or `max_width` is zero.
///
/// Each chararacter width is calculated with UnicodeWidthChar::width. If UnicodeWidthChar::width()
/// returns None, the character width is treated as 0.
pub fn substr_with_width(string: &str, max_width: usize) -> String {
    let mut result = String::new();
    let mut width = 0;
    for g in string.graphemes(true) {
        // Control chars count as width 0
        let w = UnicodeWidthStr::width(g);
        if width + w > max_width {
            break;
        }
        width += w;
        result.push_str(g);
    }
    result
}

/// Returns a longest substring fits to the given width.
/// Returns an empty string if `str` is an empty string or `max_width` is zero.
///
/// Each chararacter width is calculated with UnicodeWidthChar::width. If UnicodeWidthChar::width()
/// returns None, the character width is treated as 0. A STFL tag (e.g. `<b>`, `<foobar>`, `</>`)
/// width is treated as 0, but escaped less-than (`<>`) width is treated as 1.
pub fn substr_with_width_stfl(string: &str, max_width: usize) -> String {
    let mut result = String::new();
    let mut in_bracket = false;
    let mut tagbuf = Vec::<char>::new();
    let mut width = 0;
    for c in string.chars() {
        if in_bracket {
            tagbuf.push(c);
            if c == '>' {
                in_bracket = false;
                if tagbuf == ['<', '>'] {
                    if width + 1 > max_width {
                        break;
                    }
                    result += "<>"; // escaped less-than
                    tagbuf.clear();
                    width += 1;
                } else {
                    result += &tagbuf.iter().collect::<String>();
                    tagbuf.clear();
                }
            }
        } else if c == '<' {
            in_bracket = true;
            tagbuf.push(c);
        } else {
            // Control chars count as width 0
            let w = UnicodeWidthChar::width(c).unwrap_or(0);
            if width + w > max_width {
                break;
            }
            width += w;
            result.push(c);
        }
    }
    result
}

/// Remove all soft-hyphens as they can behave unpredictably (see
/// <https://github.com/akrennmair/newsbeuter/issues/259#issuecomment-259609490>) and inadvertently
/// render as hyphens
pub fn remove_soft_hyphens(text: &mut String) {
    text.retain(|c| c != '\u{00AD}')
}

/// An array of "MIME matchers" and their associated LinkTypes
///
/// This is used for two tasks:
///
/// 1. checking if a MIME type is a podcast type (`utils::is_valid_podcast_type`). That involves
///    running all matching functions on given input and checking if any of them returned `true`;
///
/// 2. figuring out the `LinkType` for a particular enclosure, given its MIME type
///    (`utils::podcast_mime_to_link_type`).
type MimeMatcher = (fn(&str) -> bool, links::LinkType);
const PODCAST_MIME_TO_LINKTYPE: [MimeMatcher; 2] = [
    (
        |mime| {
            // RFC 5334, section 10.1 says "historically, some implementations expect .ogg files to be
            // solely Vorbis-encoded audio", so let's assume it's audio, not video.
            // https://tools.ietf.org/html/rfc5334#section-10.1
            mime.starts_with("audio/") || mime == "application/ogg"
        },
        links::LinkType::Audio,
    ),
    (|mime| mime.starts_with("video/"), links::LinkType::Video),
];

/// Returns `true` if given MIME type is considered to be a podcast by Newsboat.
pub fn is_valid_podcast_type(mimetype: &str) -> bool {
    PODCAST_MIME_TO_LINKTYPE
        .iter()
        .any(|(matcher, _)| matcher(mimetype))
}

/// Converts podcast's MIME type into an HtmlRenderer's "link type"
///
/// Returns None if given MIME type is not a podcast type. See `is_valid_podcast_type()`.
pub fn podcast_mime_to_link_type(mime_type: &str) -> Option<links::LinkType> {
    PODCAST_MIME_TO_LINKTYPE
        .iter()
        .find_map(|(matcher, link_type)| {
            if matcher(mime_type) {
                Some(*link_type)
            } else {
                None
            }
        })
}

pub fn get_auth_method(method: &str) -> c_ulong {
    match method {
        "basic" => curl_sys::CURLAUTH_BASIC,
        "digest" => curl_sys::CURLAUTH_DIGEST,
        "digest_ie" => curl_sys::CURLAUTH_DIGEST_IE,
        "gssnegotiate" => curl_sys::CURLAUTH_GSSNEGOTIATE,
        "ntlm" => curl_sys::CURLAUTH_NTLM,
        "anysafe" => curl_sys::CURLAUTH_ANYSAFE,
        "any" | "" => curl_sys::CURLAUTH_ANY,
        _ => {
            log!(
                Level::UserError,
                "utils::get_auth_method: you configured an invalid proxy authentication method: {}",
                method
            );
            curl_sys::CURLAUTH_ANY
        }
    }
}

pub fn unescape_url(rs_str: String) -> Option<String> {
    let decoded = percent_decode(rs_str.as_bytes()).decode_utf8();
    decoded.ok().map(|s| s.replace('\0', ""))
}

/// Runs given command in a shell, and returns the output (from stdout; stderr is printed to the
/// screen).
pub fn get_command_output(cmd: &str) -> String {
    let cmd = Command::new("sh")
        .arg("-c")
        .arg(cmd)
        // Inherit stdin so that the program can ask something of the user (see
        // https://github.com/Newsboat/Newsboat/issues/455 for an example).
        .stdin(Stdio::inherit())
        .output();
    // from_utf8_lossy will convert any bad bytes to U+FFFD
    cmd.map(|cmd| String::from_utf8_lossy(&cmd.stdout).into_owned())
        .unwrap_or_else(|_| String::from(""))
}

// This function assumes that the user is not interested in the command's output (not even errors
// on stderr!), so it will close the spawned command's fds.
// This used to be a simple std::process::Command::Spawn(), but this caused child processes to not
// be reaped, hence the addition of double forking to this function.
// Spawn() was replaced with a direct call to fork+execvp because it interacted badly with the
// fork() call.
pub fn run_command(cmd: &str, param: &str) {
    unsafe {
        let forked_pid = fork();

        match forked_pid {
            -1 => {
                // Parent process, fork failed.
                log!(
                    Level::Debug,
                    "utils::run_command: failed to fork. Aborting run_command."
                );
                return;
            }
            0 => {
                // Child process, continue and spawn the command.
            }
            _ => {
                // Parent process, fork succeeded: reap child and return.
                let mut status = 0;
                if waitpid(forked_pid, &mut status, 0) == -1 {
                    log!(Level::Debug, "utils::run_command: waitpid failed.");
                }
                return;
            }
        }

        if fork() == 0 {
            // Grand-child
            match (CString::new(cmd), CString::new(param)) {
                (Ok(c_cmd), Ok(c_param)) => {
                    // close our fds to avoid clobbering the screen and exec the command
                    close(0);
                    close(1);
                    close(2);
                    let c_arg = [c_cmd.as_ptr(), c_param.as_ptr(), ptr::null()];
                    execvp(c_cmd.as_ptr(), c_arg.as_ptr());
                }
                _ => {
                    log!(
                        Level::UserError,
                        "Conversion of \"{}\" and/or \"{}\" to CString failed.",
                        cmd,
                        param
                    );
                }
            }
        }

        // Child process or grand child in case of failure to execvp, in both cases nothing to do.
        exit(0);
    }
}

pub fn run_program(cmd_with_args: &[&str], input: String) -> String {
    if cmd_with_args.is_empty() {
        return String::new();
    }

    Command::new(cmd_with_args[0])
        .args(&cmd_with_args[1..])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::null())
        .spawn()
        .map_err(|error| {
            log!(
                Level::Debug,
                "utils::run_program: spawning a child for \"{:?}\" \
                 with input \"{}\" failed: {}",
                cmd_with_args,
                &input,
                error
            );
        })
        .and_then(|mut child| {
            if let Some(mut stdin) = child.stdin.take() {
                std::thread::spawn(move || {
                    if let Err(error) = stdin.write_all(input.as_bytes()) {
                        log!(
                            Level::Debug,
                            "utils::run_program: failed to write to child's stdin: {}",
                            error
                        );
                    }
                });
            }

            child
                .wait_with_output()
                .map_err(|error| {
                    log!(
                        Level::Debug,
                        "utils::run_program: failed to read child's stdout: {}",
                        error
                    );
                })
                .map(|output| String::from_utf8_lossy(&output.stdout).into_owned())
        })
        .unwrap_or_else(|_| String::new())
}

pub fn make_title(rs_str: String) -> String {
    /* Sometimes it is possible to construct the title from the URL
     * This attempts to do just that. eg:
     * http://domain.com/story/yy/mm/dd/title-with-dashes?a=b
     */
    // Strip out trailing slashes
    let mut result = rs_str.trim_end_matches('/');

    // get to the final part of the URI's path and
    // extract just the juicy part 'title-with-dashes?a=b'
    let v: Vec<&str> = result.rsplitn(2, '/').collect();
    result = v[0];

    // find where query part of URI starts
    // throw away the query part 'title-with-dashes'
    let v: Vec<&str> = result.splitn(2, '?').collect();
    result = v[0];

    // Throw away common webpage suffixes: .html, .php, .aspx, .htm
    result = result
        .trim_end_matches(".html")
        .trim_end_matches(".php")
        .trim_end_matches(".aspx")
        .trim_end_matches(".htm");

    // 'title with dashes'
    let result = result.replace(['-', '_'], " ");

    //'Title with dashes'
    //let result = "";
    let mut c = result.chars();
    let result = match c.next() {
        None => String::new(),
        Some(f) => f.to_uppercase().collect::<String>() + c.as_str(),
    };

    // Un-escape any percent-encoding, e.g. "It%27s%202017%21" -> "It's
    // 2017!"
    let result = unescape_url(result).unwrap_or_default();

    // If it contains only digits, assume it's an id, not a proper title
    if result.chars().all(|c| c.is_ascii_digit()) {
        String::new()
    } else {
        result
    }
}

/// Run the given command interactively with inherited stdin and stdout/stderr. Return the lowest
/// 8 bits of its exit code, or `None` if the command failed to start.
pub fn run_interactively(command: &str, caller: &str) -> Option<u8> {
    log!(Level::Debug, &format!("{caller}: running `{command}'"));
    Command::new("sh")
        .arg("-c")
        .arg(command)
        .status()
        .map_err(|err| {
            log!(
                Level::Warn,
                &format!("{caller}: Couldn't create child process: {err}")
            );
        })
        .ok()
        .and_then(|exit_status| exit_status.code())
        .map(|exit_code| exit_code as u8)
}

/// Run the given command non-interactively with closed stdin and stdout/stderr. Return the lowest
/// 8 bits of its exit code, or `None` if the command failed to start.
pub fn run_non_interactively(command: &str, caller: &str) -> Option<u8> {
    log!(Level::Debug, &format!("{caller}: running `{command}'"));
    Command::new("sh")
        .arg("-c")
        .arg(command)
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status()
        .map_err(|err| {
            log!(
                Level::Warn,
                &format!("{caller}: Couldn't create child process: {err}")
            );
        })
        .ok()
        .and_then(|exit_status| exit_status.code())
        .map(|exit_code| exit_code as u8)
}

/// Get the current working directory.
pub fn getcwd() -> Result<PathBuf, io::Error> {
    use std::env;
    env::current_dir()
}

/// Errors that `read_text_file()` can return.
#[derive(Debug)]
pub enum ReadTextFileError {
    /// An error occurred while opening the file.
    CantOpen { reason: io::Error },

    /// One of the lines triggered an io::Error (most likely invalid UTF-8, but see `reason` for
    /// details).
    LineError {
        line_number: usize,
        reason: io::Error,
    },
}

/// Get the lines of text contained in a file.
pub fn read_text_file(filename: &Path) -> Result<Vec<String>, ReadTextFileError> {
    use std::fs::File;
    use std::io::BufRead;
    let file = File::open(filename).map_err(|reason| ReadTextFileError::CantOpen { reason })?;
    let buffered = io::BufReader::new(file);
    let mut lines = Vec::new();
    for (line_number, line) in buffered.lines().enumerate() {
        let line = line.map_err(|reason| ReadTextFileError::LineError {
            line_number: line_number + 1,
            reason,
        })?;
        lines.push(line);
    }
    Ok(lines)
}

pub fn strnaturalcmp(a: &str, b: &str) -> std::cmp::Ordering {
    natord::compare(a, b)
}

/// Recursively create directories if missing and set permissions accordingly.
pub fn mkdir_parents<R: AsRef<Path>>(p: &R, mode: u32) -> io::Result<()> {
    DirBuilder::new()
        .mode(mode)
        .recursive(true) // directories created with same security and permissions
        .create(p.as_ref())
}

/// The tag and Git commit ID the program was built from, or a pre-defined value from config.h if
/// there is no Git directory.
pub fn program_version() -> String {
    // NEWSBOAT_VERSION is set by this crate's build script, "build.rs"
    env!("NEWSBOAT_VERSION").to_string()
}

/// Newsboat's major version number.
pub fn Newsboat_major_version() -> u32 {
    // This will panic if the version couldn't be parsed, which is virtually impossible as Cargo
    // won't even start compilation if it couldn't parse the version.
    env!("CARGO_PKG_VERSION_MAJOR").parse::<u32>().unwrap()
}

/// Returns the part of the string before first # character (or the whole input string if there are
/// no # character in it). Pound characters inside double quotes and backticks are ignored.
pub fn strip_comments(line: &str) -> &str {
    let mut prev_was_backslash = false;
    let mut inside_quotes = false;
    let mut inside_backticks = false;

    let mut first_pound_chr_idx = line.len();

    for (idx, chr) in line.char_indices() {
        match chr {
            '\\' => {
                prev_was_backslash = true;
                continue;
            }
            '"' => {
                // If the quote is escaped or we're inside backticks, do nothing
                if !prev_was_backslash && !inside_backticks {
                    inside_quotes = !inside_quotes;
                }
            }
            '`' => {
                // If the backtick is escaped, do nothing
                if !prev_was_backslash {
                    inside_backticks = !inside_backticks;
                }
            }
            '#' => {
                if !prev_was_backslash && !inside_quotes && !inside_backticks {
                    first_pound_chr_idx = idx;
                    break;
                }
            }
            _ => {}
        }

        // We call `continue` when we run into a backslash; here, we handle all the other
        // characters, which clearly *aren't* a backslash
        prev_was_backslash = false;
    }

    &line[0..first_pound_chr_idx]
}

fn get_escape_value(c: char) -> String {
    match c {
        'n' => "\n".to_owned(),
        'r' => "\r".to_owned(),
        't' => "\t".to_owned(),
        '"' => "\"".to_owned(),
        // escaped backticks are passed through, still escaped. We un-escape
        // them in ConfigParser::evaluate_backticks
        '`' => "\\`".to_owned(),
        '\\' => "\\".to_owned(),
        x => x.to_owned().to_string(),
    }
}

pub fn extract_token_quoted<'a>(line: &'a str, delimiters: &str) -> (Option<String>, &'a str) {
    let first_non_delimiter = line.find(|c| !delimiters.contains(c));
    let line = match first_non_delimiter {
        Some(x) => &line[x..],
        None => return (None, ""),
    };

    if line.starts_with('#') {
        return (None, "");
    }

    if line.starts_with('"') {
        let mut token = String::new();
        let mut it = line.chars();
        it.next(); // Ignore opening quotation mark
        while let Some(c) = it.next() {
            if c == '"' {
                return (Some(token), it.as_str());
            } else if c == '\\' {
                if let Some(escaped_char) = it.next() {
                    token.push_str(&get_escape_value(escaped_char));
                }
            } else {
                token.push(c);
            }
        }
        (Some(token), "")
    } else {
        let end_of_token = line.find(|c| delimiters.contains(c));
        match end_of_token {
            Some(x) => (Some(line[..x].to_owned()), &line[x..]),
            None => (Some(line.to_owned()), ""),
        }
    }
}

/// Tokenize strings, obeying quotes and throwing away comments that start with a '#'
pub fn tokenize_quoted(line: &str, delimiters: &str) -> Vec<String> {
    let mut tokens = Vec::new();

    let mut todo = line;
    while !todo.is_empty() {
        let (token, remainder) = extract_token_quoted(todo, delimiters);
        if let Some(x) = token {
            tokens.push(x);
        }
        todo = remainder;
    }

    tokens
}

/// The result of executing `extract_filter()`.
pub struct FilterUrlParts {
    #[allow(rustdoc::bare_urls)]
    /// "~/bin/foo.sh" in "filter:~/bin/foo.sh:https://example.com/news.atom"
    pub script_name: String,

    #[allow(rustdoc::bare_urls)]
    /// "https://example.com/news.atom" in "filter:~/bin/foo.sh:https://example.com/news.atom"
    pub url: String,
}

/// Extract filter and url from line separated by ':'.
pub fn extract_filter(line: &str) -> FilterUrlParts {
    debug_assert!(line.starts_with("filter:"));
    // line must start with "filter:"
    let line = line.get("filter:".len()..).unwrap();
    let (script_name, url) = line.split_at(line.find(':').unwrap_or(0));
    let url = url.get(1..).unwrap_or("");
    log!(
        Level::Debug,
        "utils::extract_filter: {} -> script_name: {} url: {}",
        line,
        script_name,
        url
    );
    FilterUrlParts {
        script_name: script_name.to_string(),
        url: url.to_string(),
    }
}

// This type name comes from C, and we'd like to keep its original spelling to make the code easier
// to compare to other C code.
#[allow(non_camel_case_types)]
type iconv_t = *mut c_void;

// On FreeBSD, link with GNU libiconv; the iconv implementation in libc doesn't support //TRANSLIT
// and WCHAR_T. This is also why we change the symbol names from "iconv" to "libiconv" below.
#[cfg_attr(target_os = "freebsd", link(name = "iconv"))]
#[cfg_attr(target_os = "openbsd", link(name = "iconv"))]
extern "C" {
    #[cfg_attr(target_os = "freebsd", link_name = "libiconv_open")]
    #[cfg_attr(target_os = "openbsd", link_name = "libiconv_open")]
    pub fn iconv_open(tocode: *const c_char, fromcode: *const c_char) -> iconv_t;
    #[cfg_attr(target_os = "freebsd", link_name = "libiconv")]
    #[cfg_attr(target_os = "openbsd", link_name = "libiconv")]
    pub fn iconv(
        cd: iconv_t,
        inbuf: *mut *mut c_char,
        inbytesleft: *mut size_t,
        outbuf: *mut *mut c_char,
        outbytesleft: *mut size_t,
    ) -> size_t;
    #[cfg_attr(target_os = "freebsd", link_name = "libiconv_close")]
    #[cfg_attr(target_os = "openbsd", link_name = "libiconv_close")]
    pub fn iconv_close(cd: iconv_t) -> c_int;
}

/// Annotates the target encoding (`tocode`) with `//TRANSLIT` if conversion between `fromcode` and
/// `tocode` might require transliterating some characters.
pub fn translit(tocode: &str, fromcode: &str) -> String {
    #[derive(PartialEq)]
    enum TranslitState {
        Unknown,
        Supported,
        Unsupported,
    }
    // Mutable `static`s are unsafe because they're a global state which might suffer from races.
    // However, in our case it's okay: even if multiple threads run this function, they will all
    // arrive at the same result, so we don't care if those threads race to write that result into
    // the variable.
    static mut STATE: TranslitState = TranslitState::Unknown;

    // TRANSLIT is not needed when converting to unicode encodings
    if tocode == "utf-8" || tocode == "WCHAR_T" {
        return tocode.to_string();
    }

    let tocode_translit = format!("{tocode}//TRANSLIT");

    unsafe {
        if STATE == TranslitState::Unknown {
            // The three calls to expect() can't panic because the input strings come from either:
            //
            // - our code, and we won't deliberately put a NUL byte inside an encoding name; or
            // - the user's locale, which we obtain via C interfaces that would've stripped a NUL byte.
            let c_tocode = CString::new(tocode.to_string()).expect("tocode String contains NUL");
            let c_tocode_translit = CString::new(tocode_translit.to_string())
                .expect("tocode_translit String contains NUL");
            let c_fromcode = CString::new(fromcode).expect("fromcode String contains NUL");

            let cd = iconv_open(c_tocode_translit.as_ptr(), c_fromcode.as_ptr());

            if cd == -1_isize as iconv_t {
                let errno = std::io::Error::last_os_error()
                    .raw_os_error()
                    .expect("Error constructed with last_os_error didn't contain errno code");
                if errno == EINVAL {
                    let cd = iconv_open(c_tocode.as_ptr(), c_fromcode.as_ptr());
                    if cd != -1_isize as iconv_t {
                        STATE = TranslitState::Unsupported;
                        iconv_close(cd);
                    } else {
                        let errno = std::io::Error::last_os_error();
                        eprintln!("iconv_open('{tocode}', '{fromcode}') failed: {errno}");
                        std::process::abort();
                    }
                } else {
                    let errno = std::io::Error::last_os_error();
                    eprintln!("iconv_open('{tocode_translit}', '{fromcode}') failed: {errno}");
                    std::process::abort();
                }
            } else {
                STATE = TranslitState::Supported;
                iconv_close(cd);
            }
        }
    }

    if unsafe { STATE == TranslitState::Supported } {
        tocode_translit
    } else {
        tocode.to_string()
    }
}

/// Converts `text` from encoding `fromcode` to encoding `tocode`.
pub fn convert_text(text: &[u8], tocode: &str, fromcode: &str) -> Vec<u8> {
    let mut result = vec![];

    let tocode_translit = translit(tocode, fromcode);

    // Illegal and incomplete multi-byte sequences will be replaced by this
    // placeholder. By default, we use an ASCII value for "question mark".
    let question_mark = {
        let mut question_mark = vec![0x3f];

        // This `if` prevents the function from recursing indefinitely.
        if text != question_mark && fromcode != "ASCII" {
            question_mark = convert_text(&question_mark, &tocode_translit, "ASCII");
        }
        // If we can't even convert a question mark, let's just give up.
        if question_mark.is_empty() {
            return result;
        }

        question_mark
    };

    let cd = unsafe {
        // The following two `expect()`s can't panic because their input string come from trusted
        // sources:
        // - from our code, and we obviously won't put NUL inside one of those strings;
        // - from the locale, which we interrogate via a C API which "filters out" any NULs.
        let c_tocode_translit =
            CString::new(tocode_translit).expect("tocode_translit String contained NUL");
        let c_fromcode = CString::new(fromcode).expect("fromcode str contained NUL");

        iconv_open(c_tocode_translit.as_ptr(), c_fromcode.as_ptr())
    };

    if cd == -1isize as iconv_t {
        return result;
    }

    unsafe {
        let mut outbuf = vec![0u8; 65536];
        let mut outbufp = outbuf.as_mut_ptr() as *mut c_char;
        let mut outbytesleft = outbuf.len();

        // iconv() wants a non-const pointer to data, but we only have an immutable &[u8] for
        // input. So we copy the data to a Vec.
        let mut input = text.to_owned();
        let mut inbufp = input.as_mut_ptr() as *mut c_char;
        let mut inbytesleft = input.len();

        let copy_converted_chunk = |outbuf: &[u8],
                                    old_outbufp: *const c_char,
                                    outbufp: *const c_char,
                                    result: &mut Vec<u8>| {
            let zero = outbuf.as_ptr() as *const c_char;
            // Casts from isize to usize are okay because `zero` points to the first element of
            // `outbuf` while `old_outbufp` and `outbufp` point to the first or later element.
            let start = old_outbufp as usize - zero as usize;
            let end = outbufp as usize - zero as usize;
            result.extend_from_slice(&outbuf[start..end]);
        };

        while inbytesleft > 0 {
            let old_outbufp = outbufp;
            let rc = iconv(
                cd,
                &mut inbufp,
                &mut inbytesleft,
                &mut outbufp,
                &mut outbytesleft,
            );
            if rc == -1isize as size_t {
                let errno = std::io::Error::last_os_error()
                    .raw_os_error()
                    .expect("Error constructed with last_os_error didn't contain errno code");
                match errno {
                    E2BIG => {
                        copy_converted_chunk(&outbuf, old_outbufp, outbufp, &mut result);
                        outbufp = outbuf.as_mut_ptr() as *mut c_char;
                        outbytesleft = outbuf.len();
                    }
                    EILSEQ | EINVAL => {
                        copy_converted_chunk(&outbuf, old_outbufp, outbufp, &mut result);
                        result.extend_from_slice(&question_mark);
                        inbufp = inbufp.add(1);
                        inbytesleft -= 1;
                    }
                    _ => {}
                }
            } else {
                copy_converted_chunk(&outbuf, old_outbufp, outbufp, &mut result);
            }
        }

        iconv_close(cd);
    }

    result
}

fn get_locale_encoding() -> String {
    unsafe {
        use libc::{nl_langinfo, CODESET};
        use std::ffi::CStr;

        let codeset = CStr::from_ptr(nl_langinfo(CODESET));
        // Codeset names are ASCII, so the below expect() should never panic.
        codeset
            .to_str()
            .expect("Locale codeset name is not a valid UTF-8 string")
            .to_owned()
    }
}

/// Converts input string from UTF-8 to the locale's encoding (as detected by
/// nl_langinfo(CODESET)).
pub fn utf8_to_locale(text: &str) -> Vec<u8> {
    if text.is_empty() {
        return vec![];
    }

    convert_text(text.as_bytes(), &get_locale_encoding(), "utf-8")
}

/// Converts input string from the locale's encoding (as detected by
/// nl_langinfo(CODESET)) to UTF-8.
pub fn locale_to_utf8(text: &[u8]) -> String {
    if text.is_empty() {
        return String::new();
    }

    let converted = convert_text(text, "utf-8", &get_locale_encoding());
    String::from_utf8(converted).expect("convert_text() returned a non-UTF-8 string")
}

/// Parses a string in the form `author@example.com (Example Author)`
/// into a pair of (author, email).
/// If the pattern cannot be matched or the name is empty, the input
/// is returned completely as author with empty email.
///
/// Invalid data (outside of utf-8) is replaced with Unicode replacement characters.
pub fn parse_rss_author_email(text: &[u8]) -> (String, String) {
    let re = regex::bytes::Regex::new(r#"^([^ ]+@[^ ]+)\s+\((.+)\)$"#)
        .expect("Programming error in hardcoded regex");

    if let Some(m) = re.captures(text) {
        let (_, [email, name]) = m.extract();

        // Unwrapping is safe because the regex only matches valid (utf-8) unicode
        let (name, email) = (
            String::from_utf8(name.into()).unwrap().trim().to_owned(),
            String::from_utf8(email.into()).unwrap().trim().to_owned(),
        );

        if !name.is_empty() {
            return (name, email);
        }
    }

    (String::from_utf8_lossy(text).into_owned(), "".into())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_replace_all() {
        assert_eq!(
            replace_all(String::from("aaa"), "a", "b"),
            String::from("bbb")
        );
        assert_eq!(
            replace_all(String::from("aaa"), "aa", "ba"),
            String::from("baa")
        );
        assert_eq!(
            replace_all(String::from("aaaaaa"), "aa", "ba"),
            String::from("bababa")
        );
        assert_eq!(replace_all(String::new(), "a", "b"), String::new());

        let input = String::from("aaaa");
        assert_eq!(replace_all(input.clone(), "b", "c"), input);

        assert_eq!(
            replace_all(String::from("this is a normal test text"), " t", " T"),
            String::from("this is a normal Test Text")
        );

        assert_eq!(
            replace_all(String::from("o o o"), "o", "<o>"),
            String::from("<o> <o> <o>")
        );
    }

    #[test]
    fn t_consolidate_whitespace() {
        assert_eq!(
            consolidate_whitespace(String::from("LoremIpsum")),
            String::from("LoremIpsum")
        );
        assert_eq!(
            consolidate_whitespace(String::from("Lorem Ipsum")),
            String::from("Lorem Ipsum")
        );
        assert_eq!(
            consolidate_whitespace(String::from(" Lorem \t\tIpsum \t ")),
            String::from(" Lorem Ipsum ")
        );
        assert_eq!(
            consolidate_whitespace(String::from(" Lorem \r\n\r\n\tIpsum")),
            String::from(" Lorem Ipsum")
        );
        assert_eq!(consolidate_whitespace(String::new()), String::new());
        assert_eq!(
            consolidate_whitespace(String::from("    Lorem \t\tIpsum \t ")),
            String::from("    Lorem Ipsum ")
        );
        assert_eq!(
            consolidate_whitespace(String::from("   Lorem \r\n\r\n\tIpsum")),
            String::from("   Lorem Ipsum")
        );
    }

    #[test]
    fn t_extract_token_quoted_ignores_comments() {
        let delimiters = " \r\n\t";
        assert_eq!(
            extract_token_quoted("\t\t  # commented out", delimiters),
            (None, "")
        );

        // ignores '#' if it is inside quotes
        assert_eq!(
            extract_token_quoted(r##""# in quoted token" other tokens"##, delimiters),
            (Some(r#"# in quoted token"#.to_owned()), " other tokens")
        );

        // token before start of comment
        assert_eq!(
            extract_token_quoted(r#"some-token # some comment"#, delimiters),
            (Some("some-token".to_owned()), " # some comment")
        );
    }

    #[test]
    fn t_extract_token_quoted_ignores_delimiters_in_front_of_tokens() {
        // empty delimiters
        assert_eq!(
            extract_token_quoted("\n\r\t \n\t\r token-name", ""),
            (Some("\n\r\t \n\t\r token-name".to_owned()), "")
        );

        // single delimiter
        assert_eq!(
            extract_token_quoted("--token name--", "-"),
            (Some("token name".to_owned()), "--")
        );

        // two delimiters
        assert_eq!(
            extract_token_quoted("--token name--", " -"),
            (Some("token".to_owned()), " name--")
        );
    }

    #[test]
    fn t_extract_token_quoted_ignores_delimiters_in_quoted_strings() {
        assert_eq!(
            extract_token_quoted(r#"  "token name"  "#, " \r\n\t"),
            (Some("token name".to_owned()), "  ")
        );

        assert_eq!(
            extract_token_quoted(r#"--"token name"--"#, " -"),
            (Some("token name".to_owned()), "--")
        );
    }

    #[test]
    fn t_extract_token_quoted_processes_escape_sequences_in_quoted_strings() {
        assert_eq!(
            extract_token_quoted(r#"  "\n \r \t \" \` \\ " remainder"#, " \r\n\t"),
            (Some("\n \r \t \" \\` \\ ".to_owned()), " remainder")
        );
    }

    #[test]
    fn t_tokenize_quoted_splits_on_delimiters() {
        assert_eq!(
            tokenize_quoted("asdf \"foobar bla\" \"foo\\r\\n\\tbar\"", " \r\n\t"),
            vec!["asdf", "foobar bla", "foo\r\n\tbar"]
        );

        assert_eq!(
            tokenize_quoted("  \"foo \\\\xxx\"\t\r \" \"", " \r\n\t"),
            vec!["foo \\xxx", " "]
        );
    }

    #[test]
    fn t_tokenize_quoted_stops_at_closing_quotation_mark() {
        assert_eq!(
            tokenize_quoted(r#"set browser "mpv %u";"#, " "),
            vec!["set", "browser", "mpv %u", ";"]
        );
    }

    #[test]
    fn t_tokenize_quoted_implicitly_closes_quotes_at_end_of_string() {
        assert_eq!(tokenize_quoted("\"\\\\", " "), vec!["\\"]);

        assert_eq!(
            tokenize_quoted("\"\\\\\" and \"some other stuff", " "),
            vec!["\\", "and", "some other stuff"]
        );

        // Backslash at end of string is ignored
        assert_eq!(tokenize_quoted(r#""abc\"#, " "), vec!["abc"]);
    }

    #[test]
    fn t_tokenize_quoted_interprets_double_backslash_as_literal_backslash() {
        assert_eq!(tokenize_quoted(r#""""#, ""), vec![""]);

        assert_eq!(tokenize_quoted(r#""\\""#, ""), vec![r"\"]);

        assert_eq!(tokenize_quoted(r##""#\\""##, ""), vec!["#\\"]);

        assert_eq!(tokenize_quoted(r#""'#\\'""#, ""), vec!["'#\\'"]);

        assert_eq!(tokenize_quoted(r#""'#\\ \\'""#, ""), vec![r"'#\ \'"]);

        assert_eq!(tokenize_quoted("\"\\\\\\\\", ""), vec![r"\\"]);

        assert_eq!(tokenize_quoted("\"\\\\\\\\\\\\", ""), vec![r"\\\"]);

        assert_eq!(tokenize_quoted("\"\\\\\\\\\"", ""), vec![r"\\"]);

        assert_eq!(tokenize_quoted("\"\\\\\\\\\\\\\"", ""), vec![r"\\\"]);

        assert_eq!(tokenize_quoted(r#""\\bgit\\b""#, ""), vec![r"\bgit\b"]);

        assert_eq!(
            tokenize_quoted(
                r#"browser "/Applications/Google\\ Chrome.app/Contents/MacOS/Google\\ Chrome --app %u""#,
                " "
            ),
            vec![
                "browser",
                r"/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --app %u"
            ]
        );
    }

    #[test]
    fn t_tokenize_quoted_keeps_backticks_escaped() {
        assert_eq!(
            tokenize_quoted("asdf \"\\`foobar `bla`\\`\"", " "),
            vec!["asdf", "\\`foobar `bla`\\`"]
        );
    }

    #[test]
    fn t_tokenize_quoted_stops_at_comment_starting_with_pound_character() {
        //	A string consisting of just a comment
        assert!(tokenize_quoted("# just a comment", " ").is_empty());

        //	A string with one quoted substring
        assert_eq!(
            tokenize_quoted(r#""a test substring" # !!!"#, " "),
            vec!["a test substring"]
        );

        //	A string with two quoted substrings
        assert_eq!(
            tokenize_quoted(r#""first sub" "snd" # comment"#, " "),
            vec!["first sub", "snd"]
        );

        //	A comment containing # character
        assert_eq!(
            tokenize_quoted(r#"one # a comment with # char"#, " "),
            vec!["one"]
        );

        //	A # character inside quoted substring is ignored
        assert_eq!(
            tokenize_quoted(r#"this "will # be" ignored"#, " "),
            vec!["this", "will # be", "ignored"]
        );
    }

    #[test]
    fn t_tokenize_quoted_ignores_escaped_pound_sign_start_of_token() {
        assert_eq!(
            tokenize_quoted(r"one \# two three # ???", " "),
            vec!["one", "\\#", "two", "three"]
        );
    }

    #[test]
    fn t_extract_token_quoted_works_with_unicode_strings() {
        assert_eq!(
            extract_token_quoted(r#""привет мир" Юникода"#, " \r\n\t"),
            (Some("привет мир".to_owned()), " Юникода")
        );
    }

    #[test]
    fn t_to_u() {
        assert_eq!(to_u(String::from("0"), 10), 0);
        assert_eq!(to_u(String::from("23"), 1), 23);
        assert_eq!(to_u(String::from(""), 0), 0);
        assert_eq!(to_u(String::from("zero"), 1), 1);
    }

    #[test]
    fn t_is_special_url() {
        assert!(is_special_url("query:"));
        assert!(is_special_url("query: example"));
        assert!(!is_special_url("query"));
        assert!(!is_special_url("   query:"));

        assert!(is_special_url("filter:"));
        assert!(is_special_url("filter: example"));
        assert!(!is_special_url("filter"));
        assert!(!is_special_url("   filter:"));

        assert!(is_special_url("exec:"));
        assert!(is_special_url("exec: example"));
        assert!(!is_special_url("exec"));
        assert!(!is_special_url("   exec:"));
    }

    #[test]
    fn t_is_http_url() {
        assert!(is_http_url("http://example.com"));
        assert!(is_http_url("https://foo.bar"));
        assert!(is_http_url("http://"));
        assert!(is_http_url("https://"));

        assert!(!is_http_url("htt://foo.bar"));
        assert!(!is_http_url("http:/"));
        assert!(!is_http_url("foo://bar"));
    }

    #[test]
    fn t_is_query_url() {
        assert!(is_query_url("query:"));
        assert!(is_query_url("query: example"));

        assert!(!is_query_url("query"));
        assert!(!is_query_url("   query:"));
    }

    #[test]
    fn t_is_filter_url() {
        assert!(is_filter_url("filter:"));
        assert!(is_filter_url("filter: example"));

        assert!(!is_filter_url("filter"));
        assert!(!is_filter_url("   filter:"));
    }

    #[test]
    fn t_is_exec_url() {
        assert!(is_exec_url("exec:"));
        assert!(is_exec_url("exec: example"));

        assert!(!is_exec_url("exec"));
        assert!(!is_exec_url("   exec:"));
    }

    #[test]
    fn t_trim() {
        assert_eq!(trim(String::from("  xxx\r\n")), "xxx");
        assert_eq!(trim(String::from("\n\n abc  foobar\n")), "abc  foobar");
        assert_eq!(trim(String::from("")), "");
        assert_eq!(trim(String::from("     \n")), "");
    }

    #[test]
    fn t_trim_end() {
        assert_eq!(trim_end(String::from("quux\n")), "quux");
    }

    #[test]
    fn t_quote() {
        assert_eq!(quote("".to_string()), "\"\"");
        assert_eq!(quote("Hello World!".to_string()), "\"Hello World!\"");
        assert_eq!(
            quote("\"Hello World!\"".to_string()),
            "\"\\\"Hello World!\\\"\""
        );
    }

    #[test]
    fn t_quote_if_necessary() {
        assert_eq!(quote_if_necessary("".to_string()), "");
        assert_eq!(
            quote_if_necessary("Hello World!".to_string()),
            "\"Hello World!\""
        );
    }

    #[test]
    fn t_is_valid_color() {
        let invalid = [
            "awesome",
            "list",
            "of",
            "things",
            "that",
            "aren't",
            "colors",
            "color0123",
            "color1024",
        ];
        for color in &invalid {
            assert!(!is_valid_color(color));
        }

        let valid = [
            "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "default",
            "color0", "color163",
        ];

        for color in &valid {
            assert!(is_valid_color(color));
        }
    }

    #[test]
    fn t_strwidth() {
        assert_eq!(strwidth(""), 0);
        assert_eq!(strwidth("xx"), 2);
        assert_eq!(strwidth("\u{F91F}"), 2);
        assert_eq!(strwidth("\u{0007}"), 1);
    }

    #[test]
    fn t_strwidth_stfl() {
        assert_eq!(strwidth_stfl(""), 0);
        assert_eq!(strwidth_stfl("x<hi>x"), 2);
        assert_eq!(strwidth_stfl("x<longtag>x</>"), 2);
        assert_eq!(strwidth_stfl("x<>x"), 3);
        assert_eq!(strwidth_stfl("x<>y<>z"), 5);
        assert_eq!(strwidth_stfl("x<>hi>x"), 6);
        assert_eq!(strwidth_stfl("\u{F91F}"), 2);
        assert_eq!(strwidth_stfl("\u{0007}"), 1);
        assert_eq!(strwidth_stfl("<a"), 0); // #415
        assert_eq!(strwidth_stfl("a"), 1);
        assert_eq!(strwidth_stfl("abc<tag>def"), 6);
        assert_eq!(strwidth_stfl("less-than: <>"), 12);
        assert_eq!(strwidth_stfl("ＡＢＣＤＥＦ"), 12);
    }

    #[test]
    fn t_substr_with_width() {
        assert_eq!(substr_with_width("a", 1), "a");
        assert_eq!(substr_with_width("a", 2), "a");
        assert_eq!(substr_with_width("ab", 1), "a");
        assert_eq!(substr_with_width("abc", 1), "a");
        assert_eq!(
            substr_with_width("A\u{3042}B\u{3044}C\u{3046}", 5),
            "A\u{3042}B"
        )
    }

    #[test]
    fn t_substr_with_width_given_string_empty() {
        assert_eq!(substr_with_width("", 0), "");
        assert_eq!(substr_with_width("", 1), "");
    }

    #[test]
    fn t_substr_with_width_max_width_zero() {
        assert_eq!(substr_with_width("world", 0), "");
        assert_eq!(substr_with_width("", 0), "");
    }

    #[test]
    fn t_substr_with_width_max_width_dont_split_codepoints() {
        assert_eq!(substr_with_width("ＡＢＣＤＥＦ", 9), "ＡＢＣＤ");
        assert_eq!(substr_with_width("ＡＢＣ", 4), "ＡＢ");
        assert_eq!(substr_with_width("a>bcd", 3), "a>b");
        assert_eq!(substr_with_width("ＡＢＣＤＥ", 10), "ＡＢＣＤＥ");
        assert_eq!(substr_with_width("abc", 2), "ab");
    }

    #[test]
    fn t_substr_with_width_max_width_does_count_stfl_tag() {
        assert_eq!(substr_with_width("ＡＢＣ<b>ＤＥ</b>Ｆ", 9), "ＡＢＣ<b>");
        assert_eq!(substr_with_width("<foobar>ＡＢＣ", 4), "<foo");
        assert_eq!(substr_with_width("a<<xyz>>bcd", 3), "a<<");
        assert_eq!(substr_with_width("ＡＢＣ<b>ＤＥ", 10), "ＡＢＣ<b>");
        assert_eq!(substr_with_width("a</>b</>c</>", 2), "a<");
    }

    #[test]
    fn t_substr_with_width_max_width_count_marks_as_regular_characters() {
        assert_eq!(substr_with_width("<><><>", 2), "<>");
        assert_eq!(substr_with_width("a<>b<>c", 3), "a<>");
    }

    #[test]
    fn t_substr_with_width_max_width_non_printable() {
        assert_eq!(substr_with_width("\x01\x02abc", 1), "\x01");
    }

    #[test]
    fn t_substr_with_width_keeps_graphemes_complete() {
        let input = "a⚛️b"; // Symbol unicode: "\u{269B}\u{FE0F}"
        assert_eq!(substr_with_width(input, 1), "a");
        assert_eq!(substr_with_width(input, 2), "a");
        assert_eq!(substr_with_width(input, 3), "a⚛️");
        assert_eq!(substr_with_width(input, 4), "a⚛️b");
    }

    #[test]
    fn t_substr_with_width_stfl() {
        assert_eq!(substr_with_width_stfl("a", 1), "a");
        assert_eq!(substr_with_width_stfl("a", 2), "a");
        assert_eq!(substr_with_width_stfl("ab", 1), "a");
        assert_eq!(substr_with_width_stfl("abc", 1), "a");
        assert_eq!(
            substr_with_width_stfl("A\u{3042}B\u{3044}C\u{3046}", 5),
            "A\u{3042}B"
        )
    }

    #[test]
    fn t_substr_with_width_stfl_given_string_empty() {
        assert_eq!(substr_with_width_stfl("", 0), "");
        assert_eq!(substr_with_width_stfl("", 1), "");
    }

    #[test]
    fn t_substr_with_width_stfl_max_width_zero() {
        assert_eq!(substr_with_width_stfl("world", 0), "");
        assert_eq!(substr_with_width_stfl("", 0), "");
    }

    #[test]
    fn t_substr_with_width_stfl_max_width_dont_split_codepoints() {
        assert_eq!(
            substr_with_width_stfl("ＡＢＣ<b>ＤＥ</b>Ｆ", 9),
            "ＡＢＣ<b>Ｄ"
        );
        assert_eq!(substr_with_width_stfl("<foobar>ＡＢＣ", 4), "<foobar>ＡＢ");
        assert_eq!(substr_with_width_stfl("a<<xyz>>bcd", 3), "a<<xyz>>b"); // tag: "<<xyz>"
        assert_eq!(substr_with_width_stfl("ＡＢＣ<b>ＤＥ", 10), "ＡＢＣ<b>ＤＥ");
        assert_eq!(substr_with_width_stfl("a</>b</>c</>", 2), "a</>b</>");
    }

    #[test]
    fn t_substr_with_width_stfl_max_width_do_not_count_stfl_tag() {
        assert_eq!(
            substr_with_width_stfl("ＡＢＣ<b>ＤＥ</b>Ｆ", 9),
            "ＡＢＣ<b>Ｄ"
        );
        assert_eq!(substr_with_width_stfl("<foobar>ＡＢＣ", 4), "<foobar>ＡＢ");
        assert_eq!(substr_with_width_stfl("a<<xyz>>bcd", 3), "a<<xyz>>b"); // tag: "<<xyz>"
        assert_eq!(substr_with_width_stfl("ＡＢＣ<b>ＤＥ", 10), "ＡＢＣ<b>ＤＥ");
        assert_eq!(substr_with_width_stfl("a</>b</>c</>", 2), "a</>b</>");
    }

    #[test]
    fn t_substr_with_width_stfl_max_width_count_escaped_less_than_mark() {
        assert_eq!(substr_with_width_stfl("<><><>", 2), "<><>");
        assert_eq!(substr_with_width_stfl("a<>b<>c", 3), "a<>b");
    }

    #[test]
    fn t_substr_with_width_stfl_max_width_non_printable() {
        assert_eq!(substr_with_width_stfl("\x01\x02abc", 1), "\x01\x02a");
    }

    #[test]
    fn t_is_valid_podcast_type() {
        assert!(is_valid_podcast_type("audio/mpeg"));
        assert!(is_valid_podcast_type("audio/mp3"));
        assert!(is_valid_podcast_type("audio/x-mp3"));
        assert!(is_valid_podcast_type("audio/ogg"));
        assert!(is_valid_podcast_type("video/x-matroska"));
        assert!(is_valid_podcast_type("video/webm"));
        assert!(is_valid_podcast_type("application/ogg"));

        assert!(!is_valid_podcast_type("image/jpeg"));
        assert!(!is_valid_podcast_type("image/png"));
        assert!(!is_valid_podcast_type("text/plain"));
        assert!(!is_valid_podcast_type("application/zip"));
    }

    #[test]
    fn t_podcast_mime_to_link_type() {
        use crate::links::LinkType::*;

        assert_eq!(podcast_mime_to_link_type("audio/mpeg"), Some(Audio));
        assert_eq!(podcast_mime_to_link_type("audio/mp3"), Some(Audio));
        assert_eq!(podcast_mime_to_link_type("audio/x-mp3"), Some(Audio));
        assert_eq!(podcast_mime_to_link_type("audio/ogg"), Some(Audio));
        assert_eq!(podcast_mime_to_link_type("video/x-matroska"), Some(Video));
        assert_eq!(podcast_mime_to_link_type("video/webm"), Some(Video));
        assert_eq!(podcast_mime_to_link_type("application/ogg"), Some(Audio));

        assert_eq!(podcast_mime_to_link_type("image/jpeg"), None);
        assert_eq!(podcast_mime_to_link_type("image/png"), None);
        assert_eq!(podcast_mime_to_link_type("text/plain"), None);
        assert_eq!(podcast_mime_to_link_type("application/zip"), None);
    }

    #[test]
    fn t_is_valid_attribte() {
        let invalid = ["foo", "bar", "baz", "quux"];
        for attr in &invalid {
            assert!(!is_valid_attribute(attr));
        }

        let valid = [
            "standout",
            "underline",
            "reverse",
            "blink",
            "dim",
            "bold",
            "protect",
            "invis",
            "default",
        ];
        for attr in &valid {
            assert!(is_valid_attribute(attr));
        }
    }

    #[test]
    fn t_get_auth_method() {
        assert_eq!(get_auth_method("any"), curl_sys::CURLAUTH_ANY);
        assert_eq!(get_auth_method("ntlm"), curl_sys::CURLAUTH_NTLM);
        assert_eq!(get_auth_method("basic"), curl_sys::CURLAUTH_BASIC);
        assert_eq!(get_auth_method("digest"), curl_sys::CURLAUTH_DIGEST);
        assert_eq!(get_auth_method("digest_ie"), curl_sys::CURLAUTH_DIGEST_IE);
        assert_eq!(
            get_auth_method("gssnegotiate"),
            curl_sys::CURLAUTH_GSSNEGOTIATE
        );
        assert_eq!(get_auth_method("anysafe"), curl_sys::CURLAUTH_ANYSAFE);

        assert_eq!(get_auth_method(""), curl_sys::CURLAUTH_ANY);
        assert_eq!(get_auth_method("unknown"), curl_sys::CURLAUTH_ANY);
    }

    #[test]
    fn t_unescape_url() {
        assert!(unescape_url(String::from("foo%20bar")).unwrap() == "foo bar");
        assert!(
            unescape_url(String::from(
                "%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"
            ))
            .unwrap()
                == "!#$&'()*+,/:;=?@[]"
        );
    }

    #[test]
    fn t_get_command_output() {
        assert_eq!(
            get_command_output("ls /dev/null"),
            "/dev/null\n".to_string()
        );
        assert_eq!(
            get_command_output("a-program-that-is-guaranteed-to-not-exists"),
            "".to_string()
        );
        assert_eq!(get_command_output("echo c\" d e"), "".to_string());
    }

    #[test]
    fn t_run_command_executes_given_command_with_given_argument() {
        use std::{thread, time};
        use tempfile::TempDir;

        let tmp = TempDir::new().unwrap();
        let filepath = {
            let mut filepath = tmp.path().to_owned();
            filepath.push("sentry");
            filepath
        };
        assert!(!filepath.exists());

        run_command("touch", filepath.to_str().unwrap());

        // Busy-wait for 1000 tries of 10 milliseconds each, i.e. 10 seconds, waiting for `touch`
        // to create the file. Usually it happens quickly, and the loop exists on the first try;
        // but sometimes on CI it takes longer for `touch` to finish, so we need a slightly longer
        // wait.
        for _ in 0..100 {
            thread::sleep(time::Duration::from_millis(10));

            if filepath.exists() {
                break;
            }
        }

        assert!(filepath.exists());
    }

    #[test]
    fn t_run_command_doesnt_wait_for_the_command_to_finish() {
        use std::time::{Duration, Instant};

        let start = Instant::now();

        let five: &str = "5";
        run_command("sleep", five);

        let runtime = start.elapsed();

        assert!(runtime < Duration::from_secs(10));
    }

    #[test]
    fn t_run_program() {
        let input1 = "this is a multine-line\ntest string";
        assert_eq!(run_program(&["cat"], input1.to_owned()), input1);

        assert_eq!(
            run_program(&["echo", "-n", "hello world"], String::new()),
            "hello world"
        );
    }

    #[test]
    fn t_make_title() {
        let mut input = String::from("http://example.com/Item");
        assert_eq!(make_title(input), String::from("Item"));

        input = String::from("http://example.com/This-is-the-title");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/This_is_the_title");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.php");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.html");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.htm");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.aspx");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/this-is-the-title");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/items/misc/this-is-the-title");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/item/");
        assert_eq!(make_title(input), String::from("Item"));

        input = String::from("http://example.com/item/////////////");
        assert_eq!(make_title(input), String::from("Item"));

        input = String::from("blahscheme://example.com/this-is-the-title");
        assert_eq!(make_title(input), String::from("This is the title"));

        input = String::from("http://example.com/story/aug/title-with-dashes?a=b");
        assert_eq!(make_title(input), String::from("Title with dashes"));

        input = String::from("http://example.com/title-with-dashes?a=b&x=y&utf8=✓");
        assert_eq!(make_title(input), String::from("Title with dashes"));

        input = String::from("https://example.com/It%27s%202017%21");
        assert_eq!(make_title(input), String::from("It's 2017!"));

        input = String::from("https://example.com/?format=rss");
        assert_eq!(make_title(input), String::from(""));

        assert_eq!(make_title(String::from("")), String::from(""));
    }

    #[test]
    fn t_resolve_relative() {
        assert_eq!(
            resolve_relative(Path::new("/foo/bar"), Path::new("/baz")),
            Path::new("/baz")
        );
        assert_eq!(
            resolve_relative(Path::new("/config"), Path::new("/config/baz")),
            Path::new("/config/baz")
        );

        assert_eq!(
            resolve_relative(Path::new("/foo/bar"), Path::new("baz")),
            Path::new("/foo/baz")
        );
        assert_eq!(
            resolve_relative(Path::new("/config"), Path::new("baz")),
            Path::new("/baz")
        );
    }

    #[test]
    fn t_remove_soft_hyphens_removes_all_00ad_unicode_chars_from_a_string() {
        {
            // Does nothing if input has no soft hyphens in it
            let mut input1 = "hello world!".to_string();
            remove_soft_hyphens(&mut input1);
            assert_eq!(input1, "hello world!");
        }

        {
            // Removes *all* soft hyphens
            let mut data = "hy\u{00AD}phen\u{00AD}a\u{00AD}tion".to_string();
            remove_soft_hyphens(&mut data);
            assert_eq!(data, "hyphenation");
        }

        {
            // Removes consecutive soft hyphens
            let mut data = "don't know why any\u{00AD}\u{00AD}one would do that".to_string();
            remove_soft_hyphens(&mut data);
            assert_eq!(data, "don't know why anyone would do that");
        }

        {
            // Removes soft hyphen at the beginning of the line
            let mut data = "\u{00AD}tion".to_string();
            remove_soft_hyphens(&mut data);
            assert_eq!(data, "tion");
        }

        {
            // Removes soft hyphen at the end of the line
            let mut data = "over\u{00AD}".to_string();
            remove_soft_hyphens(&mut data);
            assert_eq!(data, "over");
        }
    }

    #[test]
    fn t_mkdir_parents() {
        use std::fs;
        use std::os::unix::fs::PermissionsExt;
        use tempfile::TempDir;

        let mode: u32 = 0o700;
        let tmp_dir = TempDir::new().unwrap();
        let path = tmp_dir.path().join("parent/dir");
        assert!(!path.exists());

        let result = mkdir_parents(&path, mode);
        assert!(result.is_ok());
        assert!(path.exists());

        let file_type_mask = 0o7777;
        let metadata = fs::metadata(&path).unwrap();
        assert_eq!(file_type_mask & metadata.permissions().mode(), mode);

        // rerun on existing directories
        let result = mkdir_parents(&path, mode);
        assert!(result.is_ok());
    }

    #[test]
    fn t_read_text_file_valid_unicode() {
        use std::fs;
        use tempfile::NamedTempFile;

        let text_file_location = NamedTempFile::new().unwrap();
        let data = "lorem ipsum\ntest1\ntest2";
        fs::write(text_file_location.path(), data).expect("unable to write test data to file");

        let lines = read_text_file(text_file_location.path()).unwrap();
        assert_eq!(lines.len(), 3);
        assert_eq!(lines[0], "lorem ipsum");
        assert_eq!(lines[1], "test1");
        assert_eq!(lines[2], "test2");
    }

    #[test]
    fn t_read_text_file_nonexistent_file() {
        use tempfile::TempDir;

        let tmp = TempDir::new().unwrap();
        let filepath = {
            let mut filepath = tmp.path().to_owned();
            filepath.push("nonexistent");
            filepath
        };
        assert!(!filepath.exists());
        match read_text_file(&filepath) {
            // Expected result.
            Err(ReadTextFileError::CantOpen { .. }) => {}

            other => panic!("Expected a result different from {:?}", other),
        }
    }

    #[test]
    fn t_read_text_file_invalid_unicode() {
        use std::fs;
        use tempfile::NamedTempFile;

        let text_file_location = NamedTempFile::new().unwrap();
        let data: Vec<u8> = vec![
            0x74, 0x65, 0x73, 0x74, 0x31, 0x0a, 0x74, 0xff, 0x73, 0x74, 0x32, 0x0a,
        ];
        fs::write(text_file_location.path(), data).expect("unable to write test data to file");

        match read_text_file(text_file_location.path()) {
            // Expected result.
            Err(ReadTextFileError::LineError { .. }) => {}

            other => panic!("Expected a result different from {:?}", other),
        }
    }

    #[test]
    fn t_strnaturalcmp() {
        use std::cmp::Ordering;
        assert_eq!(strnaturalcmp("", ""), Ordering::Equal);
        assert_eq!(strnaturalcmp("", "a"), Ordering::Less);
        assert_eq!(strnaturalcmp("a", ""), Ordering::Greater);
        assert_eq!(strnaturalcmp("a", "a"), Ordering::Equal);
        assert_eq!(strnaturalcmp("", "9"), Ordering::Less);
        assert_eq!(strnaturalcmp("9", ""), Ordering::Greater);
        assert_eq!(strnaturalcmp("1", "1"), Ordering::Equal);
        assert_eq!(strnaturalcmp("1", "2"), Ordering::Less);
        assert_eq!(strnaturalcmp("3", "2"), Ordering::Greater);
        assert_eq!(strnaturalcmp("a1", "a1"), Ordering::Equal);
        assert_eq!(strnaturalcmp("a1", "a2"), Ordering::Less);
        assert_eq!(strnaturalcmp("a2", "a1"), Ordering::Greater);
        assert_eq!(strnaturalcmp("a1a2", "a1a3"), Ordering::Less);
        assert_eq!(strnaturalcmp("a1a2", "a1a0"), Ordering::Greater);
        assert_eq!(strnaturalcmp("134", "122"), Ordering::Greater);
        assert_eq!(strnaturalcmp("12a3", "12a3"), Ordering::Equal);
        assert_eq!(strnaturalcmp("12a1", "12a0"), Ordering::Greater);
        assert_eq!(strnaturalcmp("12a1", "12a2"), Ordering::Less);
        assert_eq!(strnaturalcmp("a", "aa"), Ordering::Less);
        assert_eq!(strnaturalcmp("aaa", "aa"), Ordering::Greater);
        assert_eq!(strnaturalcmp("Alpha 2", "Alpha 2"), Ordering::Equal);
        assert_eq!(strnaturalcmp("Alpha 2", "Alpha 2A"), Ordering::Less);
        assert_eq!(strnaturalcmp("Alpha 2 B", "Alpha 2"), Ordering::Greater);

        assert_eq!(strnaturalcmp("aa10", "aa2"), Ordering::Greater);
    }

    #[test]
    fn t_strip_comments() {
        // no comments in line
        assert_eq!(strip_comments(""), "");
        assert_eq!(strip_comments("\t\n"), "\t\n");
        assert_eq!(strip_comments("some directive "), "some directive ");

        // fully commented line
        assert_eq!(strip_comments("#"), "");
        assert_eq!(strip_comments("# #"), "");
        assert_eq!(strip_comments("# comment"), "");

        // partially commented line
        assert_eq!(strip_comments("directive # comment"), "directive ");
        assert_eq!(
            strip_comments("directive # comment # another"),
            "directive "
        );
        assert_eq!(strip_comments("directive#comment"), "directive");

        // ignores # characters inside double quotes (#652)
        let expected = r#"highlight article "[-=+#_*~]{3,}.*" green default"#;
        let input = expected.to_owned() + "# this is a comment";
        assert_eq!(strip_comments(&input), expected);

        let expected =
            r#"highlight all "(https?|ftp)://[\-\.,/%~_:?&=\#a-zA-Z0-9]+" blue default bold"#;
        let input = expected.to_owned() + "#heresacomment";
        assert_eq!(strip_comments(&input), expected);

        // Escaped double quote inside double quotes is not treated as closing quote
        let expected = r#"test "here \"goes # nothing\" etc" hehe"#;
        let input = expected.to_owned() + "# and here is a comment";
        assert_eq!(strip_comments(&input), expected);

        // Ignores # characters inside backticks
        let expected = r#"one `two # three` four"#;
        let input = expected.to_owned() + "# and a comment, of course";
        assert_eq!(strip_comments(&input), expected);

        // Escaped backtick inside backticks is not treated as closing
        let expected = r"some `other \` tricky # test` hehe";
        let input = expected.to_owned() + "#here goescomment";
        assert_eq!(strip_comments(&input), expected);

        // Ignores escaped # characters (\\#)
        let expected = r"one two \# three four";
        let input = expected.to_owned() + "# and a comment";
        assert_eq!(strip_comments(&input), expected);
    }

    #[test]
    fn t_extract_filter() {
        let input = "filter:~/bin/script.sh:https://Newsboat.org";
        let expected_script_name = "~/bin/script.sh";
        let expected_url = "https://Newsboat.org";
        let actual = extract_filter(input);
        assert_eq!(actual.script_name, expected_script_name);
        assert_eq!(actual.url, expected_url);

        let input = "filter::https://Newsboat.org";
        let expected_script_name = "";
        let expected_url = "https://Newsboat.org";
        let actual = extract_filter(input);
        assert_eq!(actual.script_name, expected_script_name);
        assert_eq!(actual.url, expected_url);

        let input = "filter:https://Newsboat.org";
        let expected_script_name = "https";
        let expected_url = "//Newsboat.org";
        let actual = extract_filter(input);
        assert_eq!(actual.script_name, expected_script_name);
        assert_eq!(actual.url, expected_url);

        let input = "filter:foo:";
        let expected_script_name = "foo";
        let expected_url = "";
        let actual = extract_filter(input);
        assert_eq!(actual.script_name, expected_script_name);
        assert_eq!(actual.url, expected_url);

        let input = "filter:";
        let expected_script_name = "";
        let expected_url = "";
        let actual = extract_filter(input);
        assert_eq!(actual.script_name, expected_script_name);
        assert_eq!(actual.url, expected_url);
    }

    #[test]
    fn t_translit_returns_tocode_maybe_with_translit_appended() {
        // The behaviour of translit() is inherently platform-dependent: some
        // platforms support transliteration for some encodings, while others
        // support it for different encodings, or don't support it at all. This
        // tests checks just the bare minimum: the function doesn't crash, doesn't
        // throw exceptions, and returns something resembling a correct value.

        let check = |fromcode, tocode| {
            let expected1 = tocode;
            let expected2 = format!("{tocode}//TRANSLIT");

            let actual = translit(tocode, fromcode);

            assert!(actual == expected1 || actual == expected2);
        };

        check("ISO-8859-1", "UTF-8");
        check("KOI8-R", "UTF-8");
        check("UTF-16", "UTF-8");
        check("UTF-8", "UTF-8");
        check("UTF-8", "UTF-16");
        check("UTF-8", "KOI8-R");
        check("UTF-8", "ISO-8859-1");
    }

    #[test]
    fn t_translit_always_returns_the_same_value_for_the_same_inputs() {
        use std::collections::BTreeMap;

        let encodings = vec!["UTF-8", "UTF-16", "KOI8-R", "ISO-8859-1"];

        // (fromcode, tocode) -> result
        let mut results = BTreeMap::<(String, String), String>::new();

        for fromcode in &encodings {
            for tocode in &encodings {
                let key = (fromcode.to_string(), tocode.to_string());
                let result = translit(tocode, fromcode);
                results.insert(key, result);
            }
        }

        for fromcode in &encodings {
            for tocode in &encodings {
                let key = (fromcode.to_string(), tocode.to_string());
                let expected = results.get(&key).unwrap();
                let actual = translit(tocode, fromcode);
                assert_eq!(actual, *expected);
            }
        }
    }

    #[test]
    fn t_convert_text_returns_input_string_if_fromcode_and_tocode_are_the_same() {
        // \x81 is not valid UTF-8
        let input = &[0x81, 0x13, 0x41];
        let expected = &[0x3f, 0x13, 0x41];
        assert_eq!(convert_text(input, "UTF-8", "UTF-8"), expected);
    }

    #[test]
    fn t_convert_text_replaces_incomplete_multibyte_sequences_with_a_question_mark_utf8_to_utf16le()
    {
        // "ой", "oops" in Russian, but the last byte is missing
        let input = &[0xd0, 0xbe, 0xd0];
        let expected = &[0x3e, 0x04, 0x3f, 0x00];
        assert_eq!(convert_text(input, "UTF-16LE", "UTF-8"), expected);
    }

    #[test]
    fn t_convert_text_replaces_incomplete_multibyte_sequences_with_a_question_mark_utf16le_to_utf8()
    {
        // Input contains zero bytes
        // "hi", but the last byte is missing
        let input = &[0x68, 0x00, 0x69];
        let expected = &[0x68, 0x3f];
        assert_eq!(convert_text(input, "UTF-8", "UTF-16LE"), expected);

        // Input doesn't contain zero bytes
        // "эй", "hey" in Russian, but the last byte is missing
        let input = &[0x4d, 0x04, 0x39];
        let expected = &[0xd1, 0x8d, 0x3f];
        assert_eq!(convert_text(input, "UTF-8", "UTF-16LE"), expected);
    }

    #[test]
    fn t_convert_text_replaces_invalid_multibyte_sequences_with_a_question_mark_utf8_to_utf16le() {
        // "日本", "Japan", but the third byte of the first character (0xa5) is
        // missing, making the whole first character an illegal sequence.
        let input = [0xe6, 0x97, 0xe6, 0x9c, 0xac];
        let expected = [0x3f, 0x00, 0x3f, 0x00, 0x2c, 0x67];
        assert_eq!(convert_text(&input, "UTF-16LE", "UTF-8"), expected);
    }

    #[test]
    fn t_convert_text_replaces_invalid_multibyte_sequences_with_a_question_mark_utf16le_to_utf8() {
        // The first two bytes here are part of a surrogate pair, i.e. they
        // imply that the next two bytes encode additional info. However, the
        // next two bytes are an ordinary character. This breaks the decoding
        // process, so some things get turned into a question mark while others
        // are decoded incorrectly.
        let input = [0x01, 0xd8, 0xd7, 0x03];
        let expected = [0x3f, 0xed, 0x9f, 0x98, 0x3f];
        assert_eq!(convert_text(&input, "UTF-8", "UTF-16LE"), expected);
    }

    #[test]
    fn t_convert_text_converts_text_between_encodings_utf8_to_utf16le() {
        // "Тестирую", "Testing" in Russian.
        let input = [
            0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xb8, 0xd1, 0x80, 0xd1, 0x83,
            0xd1, 0x8e,
        ];
        let expected = [
            0x22, 0x04, 0x35, 0x04, 0x41, 0x04, 0x42, 0x04, 0x38, 0x04, 0x40, 0x04, 0x43, 0x04,
            0x4e, 0x04,
        ];
        assert_eq!(convert_text(&input, "UTF-16LE", "UTF-8"), expected);
    }

    #[test]
    fn t_convert_text_converts_text_between_encodings_utf8_to_koi8r() {
        // "Проверка", "Check" in Russian.
        let input = [
            0xd0, 0x9f, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xb2, 0xd0, 0xb5, 0xd1, 0x80, 0xd0, 0xba,
            0xd0, 0xb0,
        ];
        let expected = [0xf0, 0xd2, 0xcf, 0xd7, 0xc5, 0xd2, 0xcb, 0xc1];
        assert_eq!(convert_text(&input, "KOI8-R", "UTF-8"), expected);
    }

    #[test]
    fn t_convert_text_converts_text_between_encodings_utf8_to_iso8859_1() {
        // Some symbols in the result will be transliterated.

        // "вау °±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃ": a mix of Cyrillic (unsupported by
        // ISO-8859-1) and ISO-8859-1 characters.
        //
        // Have to specify the type explicitly, otherwise Rust remembers the size of the
        // array and fails to compile `assert_ne!` below, as it can't compare arrays longer than 32
        // elements.
        let input: &[u8] = &[
            0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x83, 0x20, 0xc2, 0xb0, 0xc2, 0xb1, 0xc2, 0xb2, 0xc2,
            0xb3, 0xc2, 0xb4, 0xc2, 0xb5, 0xc2, 0xb6, 0xc2, 0xb7, 0xc2, 0xb8, 0xc2, 0xb9, 0xc2,
            0xba, 0xc2, 0xbb, 0xc2, 0xbc, 0xc2, 0xbd, 0xc2, 0xbe, 0xc2, 0xbf, 0xc3, 0x80, 0xc3,
            0x81, 0xc3, 0x82, 0xc3, 0x83,
        ];

        let result = convert_text(input, "ISO-8859-1", "UTF-8");
        // We can't spell out an expected result because different platforms
        // might follow different transliteration rules.
        assert_ne!(result, &[]);
        assert_ne!(result, input);
    }

    #[test]
    fn t_convert_text_converts_text_between_encodings_utf16le_to_utf8() {
        // "Успех", "Success" in Russian.
        let input = [
            0xff, 0xfe, 0x23, 0x04, 0x41, 0x04, 0x3f, 0x04, 0x35, 0x04, 0x45, 0x04,
        ];
        let expected = [
            0xef, 0xbb, 0xbf, 0xd0, 0xa3, 0xd1, 0x81, 0xd0, 0xbf, 0xd0, 0xb5, 0xd1, 0x85,
        ];
        assert_eq!(convert_text(&input, "UTF-8", "UTF-16LE"), expected);
    }

    #[test]
    fn t_convert_text_converts_text_between_encodings_koi8r_to_utf8() {
        // "История", "History" in Russian.
        let input = [0xe9, 0xd3, 0xd4, 0xcf, 0xd2, 0xc9, 0xd1];
        let expected = [
            0xd0, 0x98, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe, 0xd1, 0x80, 0xd0, 0xb8, 0xd1, 0x8f,
        ];
        assert_eq!(convert_text(&input, "UTF-8", "KOI8-R"), expected);
    }

    #[test]
    fn t_convert_text_converts_text_between_encodings_iso8859_1_to_utf8() {
        // "ÄÅÆÇÈÉÊËÌÍÎÏ": some umlauts and Latin letters.
        let input = [
            0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        ];
        let expected = [
            0xc3, 0x84, 0xc3, 0x85, 0xc3, 0x86, 0xc3, 0x87, 0xc3, 0x88, 0xc3, 0x89, 0xc3, 0x8a,
            0xc3, 0x8b, 0xc3, 0x8c, 0xc3, 0x8d, 0xc3, 0x8e, 0xc3, 0x8f,
        ];
        assert_eq!(convert_text(&input, "UTF-8", "ISO-8859-1"), expected);
    }

    #[test]
    fn t_absolute_url() {
        assert_eq!(
            absolute_url("http://foobar/hello/crook/", "bar.html"),
            "http://foobar/hello/crook/bar.html".to_owned()
        );
        assert_eq!(
            absolute_url("https://foobar/foo/", "/bar.html"),
            "https://foobar/bar.html".to_owned()
        );
        assert_eq!(
            absolute_url("https://foobar/foo/", "http://quux/bar.html"),
            "http://quux/bar.html".to_owned()
        );
        assert_eq!(
            absolute_url("http://foobar", "bla.html"),
            "http://foobar/bla.html".to_owned()
        );
        assert_eq!(
            absolute_url("http://test:test@foobar:33", "bla2.html"),
            "http://test:test@foobar:33/bla2.html".to_owned()
        );
        assert_eq!(absolute_url("foo", "bar"), "bar".to_owned());

        // Strips ASCII whitespace: tab, line feed, form feed, carriage return, and space
        assert_eq!(
            absolute_url(" \t https://example.com/page1", "page2"),
            "https://example.com/page2".to_owned()
        );
        assert_eq!(
            absolute_url("https://example.com/page3  \n ", "page4"),
            "https://example.com/page4".to_owned()
        );
        assert_eq!(
            absolute_url("  \rhttps://example.com/page5  \x0c ", "page6"),
            "https://example.com/page6".to_owned()
        );
        assert_eq!(
            absolute_url("https://example.com/base", " \n replacement"),
            "https://example.com/replacement".to_owned()
        );
        assert_eq!(
            absolute_url("https://example.com/different", "   another\t"),
            "https://example.com/another".to_owned()
        );
        assert_eq!(
            absolute_url("https://example.com/~joe/", " \rhello\x0c"),
            "https://example.com/~joe/hello".to_owned()
        );
        assert_eq!(
            absolute_url(
                "\x0c\n\rhttps://example.com/misc/\t    ",
                "   \t\x0ceverything_at_once\n\r"
            ),
            "https://example.com/misc/everything_at_once".to_owned()
        );
    }

    #[test]
    fn t_run_non_interactively() {
        let result = run_non_interactively("echo true", "test");
        assert_eq!(result, Some(0));

        let result = run_non_interactively("exit 1", "test");
        assert_eq!(result, Some(1));

        // Unfortunately, there is no easy way to provoke this function to return `None`, nor to test
        // that it returns just the lowest 8 bits.
    }

    #[test]
    fn t_run_interactively() {
        let result = run_interactively("echo true", "test");
        assert_eq!(result, Some(0));

        let result = run_interactively("exit 1", "test");
        assert_eq!(result, Some(1));

        // Unfortunately, there is no easy way to provoke this function to return `None`, nor to test
        // that it returns just the lowest 8 bits.
    }

    #[test]
    fn t_get_basename() {
        assert_eq!(get_basename("https://example.com/"), "");
        assert_eq!(
            get_basename("https://example.org/?param=value#fragment"),
            ""
        );
        assert_eq!(
            get_basename("https://example.org/path/to/?param=value#fragment"),
            ""
        );
        assert_eq!(get_basename("https://example.org/file.mp3"), "file.mp3");
        assert_eq!(
            get_basename("https://example.org/path/to/file.mp3?param=value#fragment"),
            "file.mp3"
        );
    }

    #[test]
    fn t_quote_for_stfl() {
        assert_eq!(&quote_for_stfl("<"), "<>");
        assert_eq!(&quote_for_stfl("<<><><><"), "<><>><>><>><>");
        assert_eq!(&quote_for_stfl("test"), "test");
    }

    #[test]
    fn t_censor_url() {
        assert_eq!(&censor_url(""), "");
        assert_eq!(&censor_url("foobar"), "foobar");
        assert_eq!(&censor_url("foobar://xyz/"), "foobar://xyz/");
        assert_eq!(
            &censor_url("http://newsbeuter.org/"),
            "http://newsbeuter.org/"
        );
        assert_eq!(
            &censor_url("https://newsbeuter.org/"),
            "https://newsbeuter.org/"
        );

        assert_eq!(
            &censor_url("http://@newsbeuter.org/"),
            "http://newsbeuter.org/"
        );
        assert_eq!(
            &censor_url("https://@newsbeuter.org/"),
            "https://newsbeuter.org/"
        );

        assert_eq!(
            &censor_url("http://foo:bar@newsbeuter.org/"),
            "http://*:*@newsbeuter.org/"
        );
        assert_eq!(
            &censor_url("https://foo:bar@newsbeuter.org/"),
            "https://*:*@newsbeuter.org/"
        );

        assert_eq!(
            &censor_url("http://aschas@newsbeuter.org/"),
            "http://*:*@newsbeuter.org/"
        );
        assert_eq!(
            &censor_url("https://aschas@newsbeuter.org/"),
            "https://*:*@newsbeuter.org/"
        );

        assert_eq!(
            &censor_url("xxx://aschas@newsbeuter.org/"),
            "xxx://*:*@newsbeuter.org/"
        );

        assert_eq!(&censor_url("http://foobar"), "http://foobar/");
        assert_eq!(&censor_url("https://foobar"), "https://foobar/");

        assert_eq!(&censor_url("http://aschas@host"), "http://*:*@host/");
        assert_eq!(&censor_url("https://aschas@host"), "https://*:*@host/");

        assert_eq!(
            &censor_url("query:name:age between 1:10"),
            "query:name:age between 1:10"
        );
    }

    #[test]
    fn t_parse_rss_author_email() {
        assert_eq!(
            parse_rss_author_email(b"Example Author"),
            ("Example Author".into(), "".into())
        );

        assert_eq!(
            parse_rss_author_email(b"author@example.com (Example Author)"),
            ("Example Author".into(), "author@example.com".into())
        );

        assert_eq!(
            parse_rss_author_email(b"author@example.com  \t  (Example Author)"),
            ("Example Author".into(), "author@example.com".into())
        );

        assert_eq!(
            parse_rss_author_email(b"Wrong Order (author@example.com)"),
            ("Wrong Order (author@example.com)".into(), "".into())
        );

        assert_eq!(
            parse_rss_author_email(b"author@example.com (    )"),
            ("author@example.com (    )".into(), "".into())
        );

        assert_eq!(
            parse_rss_author_email(b"author@example.com Example Author"),
            ("author@example.com Example Author".into(), "".into())
        );

        assert_eq!(
            parse_rss_author_email(b"A Name (with extra info)"),
            ("A Name (with extra info)".into(), "".into())
        );

        assert_eq!(
            parse_rss_author_email(b"author@example.com (email) extra info"),
            ("author@example.com (email) extra info".into(), "".into())
        );
    }

    #[test]
    fn t_parse_rss_author_email_invalid_unicode() {
        // Inject invalid 0x80 byte to check that invalid unicode is not an issue
        assert_eq!(
            parse_rss_author_email(b"author\x80@example.com (Example author)"),
            (
                "author\u{fffd}@example.com (Example author)".into(),
                "".into()
            )
        );

        assert_eq!(
            parse_rss_author_email(b"author@example.com (Example \x80 author)"),
            (
                "author@example.com (Example \u{fffd} author)".into(),
                "".into()
            )
        );
    }
}
