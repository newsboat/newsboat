extern crate curl_sys;
extern crate dirs;
extern crate libc;
extern crate natord;
extern crate rand;
extern crate regex;
extern crate std;
extern crate unicode_segmentation;
extern crate unicode_width;
extern crate url;

use self::regex::Regex;
use self::unicode_segmentation::UnicodeSegmentation;
use self::unicode_width::UnicodeWidthStr;
use self::url::percent_encoding::*;
use self::url::Url;
use libc::c_ulong;
use logger::{self, Level};
use std::cmp::max;
use std::fs::DirBuilder;
use std::io::{self, Write};
use std::os::unix::fs::DirBuilderExt;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

pub fn replace_all(input: String, from: &str, to: &str) -> String {
    input.replace(from, to)
}

pub fn consolidate_whitespace(input: String) -> String {
    let found = input.find(|c: char| !c.is_whitespace());
    let mut result = String::new();

    if let Some(found) = found {
        let (leading, rest) = input.split_at(found);
        let lastchar = input.chars().rev().next().unwrap();

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
    let mut result = rs_str.parse::<u32>();

    if result.is_err() {
        result = Ok(default_value);
    }

    result.unwrap()
}

/// Combine a base URL and a link to a new absolute URL.
/// If the base URL is malformed or joining with the link fails, link will be returned.
/// # Examples
/// ```
/// use libnewsboat::utils::absolute_url;
/// assert_eq!(absolute_url("http://foobar/hello/crook/", "bar.html"),
///     "http://foobar/hello/crook/bar.html".to_owned());
/// assert_eq!(absolute_url("https://foobar/foo/", "/bar.html"),
///     "https://foobar/bar.html".to_owned());
/// assert_eq!(absolute_url("https://foobar/foo/", "http://quux/bar.html"),
///     "http://quux/bar.html".to_owned());
/// assert_eq!(absolute_url("http://foobar", "bla.html"),
///     "http://foobar/bla.html".to_owned());
/// assert_eq!(absolute_url("http://test:test@foobar:33", "bla2.html"),
///     "http://test:test@foobar:33/bla2.html".to_owned());
/// assert_eq!(absolute_url("foo", "bar"), "bar".to_owned());
/// ```
pub fn absolute_url(base_url: &str, link: &str) -> String {
    Url::parse(base_url)
        .and_then(|url| url.join(link))
        .as_ref()
        .map(Url::as_str)
        .unwrap_or(link)
        .to_owned()
}

pub fn resolve_tilde(path: String) -> String {
    let mut file_path: String = path;
    let home_path = dirs::home_dir();

    if let Some(home_path) = home_path {
        let home_path_string = home_path.to_string_lossy().into_owned();

        if file_path == "~" {
            file_path = home_path_string;
        } else {
            let tmp_file_path = file_path.clone();

            if tmp_file_path.len() > 1 {
                let (tilde, remaining) = tmp_file_path.split_at(2);

                if tilde == "~/" {
                    file_path = home_path_string + "/" + remaining;
                }
            }
        }
    }
    file_path
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
/// # Example
/// ```
/// use libnewsboat::utils::is_http_url;
/// assert!(is_http_url("http://example.com"));
/// ```
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
/// ```
/// use libnewsboat::utils::censor_url;
/// assert_eq!(&censor_url(""), "");
/// assert_eq!(&censor_url("foobar"), "foobar");
/// assert_eq!(&censor_url("foobar://xyz/"), "foobar://xyz/");
/// assert_eq!(&censor_url("http://newsbeuter.org/"),
///     "http://newsbeuter.org/");
/// assert_eq!(&censor_url("https://newsbeuter.org/"),
///     "https://newsbeuter.org/");
///
/// assert_eq!(&censor_url("http://@newsbeuter.org/"),
///         "http://newsbeuter.org/");
/// assert_eq!(&censor_url("https://@newsbeuter.org/"),
///         "https://newsbeuter.org/");
///
/// assert_eq!(&censor_url("http://foo:bar@newsbeuter.org/"),
///     "http://*:*@newsbeuter.org/");
/// assert_eq!(&censor_url("https://foo:bar@newsbeuter.org/"),
///     "https://*:*@newsbeuter.org/");
///
/// assert_eq!(&censor_url("http://aschas@newsbeuter.org/"),
///     "http://*:*@newsbeuter.org/");
/// assert_eq!(&censor_url("https://aschas@newsbeuter.org/"),
///     "https://*:*@newsbeuter.org/");
///
/// assert_eq!(&censor_url("xxx://aschas@newsbeuter.org/"),
///     "xxx://*:*@newsbeuter.org/");
///
/// assert_eq!(&censor_url("http://foobar"), "http://foobar/");
/// assert_eq!(&censor_url("https://foobar"), "https://foobar/");
///
/// assert_eq!(&censor_url("http://aschas@host"), "http://*:*@host/");
/// assert_eq!(&censor_url("https://aschas@host"), "https://*:*@host/");
///
/// assert_eq!(&censor_url("query:name:age between 1:10"),
///     "query:name:age between 1:10");
/// ```
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
/// ```
/// use libnewsboat::utils::quote_for_stfl;
/// assert_eq!(&quote_for_stfl("<"), "<>");
/// assert_eq!(&quote_for_stfl("<<><><><"), "<><>><>><>><>");
/// assert_eq!(&quote_for_stfl("test"), "test");
/// ```
pub fn quote_for_stfl(string: &str) -> String {
    return string.replace("<", "<>");
}

/// Get basename from a URL if available else return an empty string
/// ```
/// use libnewsboat::utils::get_basename;
/// assert_eq!(get_basename("https://example.com/"), "");
/// assert_eq!(get_basename("https://example.org/?param=value#fragment"), "");
/// assert_eq!(get_basename("https://example.org/path/to/?param=value#fragment"), "");
/// assert_eq!(get_basename("https://example.org/file.mp3"), "file.mp3");
/// assert_eq!(get_basename("https://example.org/path/to/file.mp3?param=value#fragment"), "file.mp3");
/// ```
pub fn get_basename(input: &str) -> String {
    match Url::parse(input) {
        Ok(url) => match url.path_segments() {
            Some(segments) => segments.last().unwrap().to_string(),
            None => String::from(""),
        },
        Err(_) => String::from(""),
    }
}

pub fn get_default_browser() -> String {
    use std::env;
    env::var("BROWSER").unwrap_or_else(|_| "lynx".to_string())
}

pub fn trim(rs_str: String) -> String {
    rs_str.trim().to_string()
}

pub fn trim_end(rs_str: String) -> String {
    let x: &[_] = &['\n', '\r'];
    rs_str.trim_end_matches(x).to_string()
}

pub fn quote(input: String) -> String {
    let mut input = input.replace("\"", "\\\"");
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
    rand::random::<u32>() % max
}

pub fn is_valid_color(color: &str) -> bool {
    const COLORS: [&str; 9] = [
        "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "default",
    ];

    if COLORS.contains(&color) {
        true
    } else if color.starts_with("color0") {
        color == "color0"
    } else if color.starts_with("color") {
        let num_part = &color[5..];
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

pub fn strwidth_stfl(rs_str: &str) -> usize {
    let reduce = 3 * rs_str
        .chars()
        .zip(rs_str.chars().skip(1))
        .filter(|&(c, next_c)| c == '<' && next_c != '>')
        .count();

    let width = strwidth(rs_str);
    if width < reduce {
        0
    } else {
        width - reduce
    }
}

/// Remove all soft-hyphens as they can behave unpredictably (see
/// https://github.com/akrennmair/newsbeuter/issues/259#issuecomment-259609490) and inadvertently
/// render as hyphens
pub fn remove_soft_hyphens(text: &mut String) {
    text.retain(|c| c != '\u{00AD}')
}

pub fn is_valid_podcast_type(mimetype: &str) -> bool {
    let re = Regex::new(r"(audio|video)/.*").unwrap();
    let matches = re.is_match(mimetype);

    let acceptable = ["application/ogg"];
    let found = acceptable.contains(&mimetype);

    matches || found
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
    decoded.ok().map(|s| s.replace("\0", ""))
}

/// Runs given command in a shell, and returns the output (from stdout; stderr is printed to the
/// screen).
pub fn get_command_output(cmd: &str) -> String {
    let cmd = Command::new("sh")
        .arg("-c")
        .arg(cmd)
        // Inherit stdin so that the program can ask something of the user (see
        // https://github.com/newsboat/newsboat/issues/455 for an example).
        .stdin(Stdio::inherit())
        .output();
    // from_utf8_lossy will convert any bad bytes to U+FFFD
    cmd.map(|cmd| String::from_utf8_lossy(&cmd.stdout).into_owned())
        .unwrap_or_else(|_| String::from(""))
}

// This function assumes that the user is not interested in command's output (not even errors on
// stderr!), so it redirects everything to /dev/null.
pub fn run_command(cmd: &str, param: &str) {
    let child = Command::new(cmd)
        .arg(param)
        // Prevent the command from blocking Newsboat by asking for input
        .stdin(Stdio::null())
        // Prevent the command from botching the screen by printing onto it.
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn();
    if let Err(error) = child {
        log!(
            Level::Debug,
            "utils::run_command: spawning a child for \"{}\" failed: {}",
            cmd,
            error
        );
    }

    // We deliberately *don't* wait for the child to finish.
}

pub fn run_program(cmd_with_args: &[&str], input: &str) -> String {
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
                input,
                error
            );
        })
        .and_then(|mut child| {
            if let Some(stdin) = child.stdin.as_mut() {
                if let Err(error) = stdin.write_all(input.as_bytes()) {
                    log!(
                        Level::Debug,
                        "utils::run_program: failed to write to child's stdin: {}",
                        error
                    );
                }
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
    let result = result.replace('-', " ").replace('_', " ");

    //'Title with dashes'
    //let result = "";
    let mut c = result.chars();
    let result = match c.next() {
        None => String::new(),
        Some(f) => f.to_uppercase().collect::<String>() + c.as_str(),
    };

    // Un-escape any percent-encoding, e.g. "It%27s%202017%21" -> "It's
    // 2017!"
    match unescape_url(result) {
        None => String::new(),
        Some(f) => f,
    }
}

/// Get the current working directory.
pub fn getcwd() -> Result<PathBuf, io::Error> {
    use std::env;
    env::current_dir()
}

pub fn strnaturalcmp(a: &str, b: &str) -> std::cmp::Ordering {
    natord::compare(a, b)
}

/// Calculate the number of padding tabs when formatting columns
///
/// The number of tabs will be adjusted by the width of the given string.  Usually, a column will
/// consist of 4 tabs, 8 characters each.  Each column will consist of at least one tab.
///
/// ```
/// use libnewsboat::utils::gentabs;
///
/// fn genstring(len: usize) -> String {
///     return std::iter::repeat("a").take(len).collect::<String>();
/// }
///
/// assert_eq!(gentabs(""), 4);
/// assert_eq!(gentabs("a"), 4);
/// assert_eq!(gentabs("aa"), 4);
/// assert_eq!(gentabs("aaa"), 4);
/// assert_eq!(gentabs("aaaa"), 4);
/// assert_eq!(gentabs("aaaaa"), 4);
/// assert_eq!(gentabs("aaaaaa"), 4);
/// assert_eq!(gentabs("aaaaaaa"), 4);
/// assert_eq!(gentabs("aaaaaaaa"), 3);
/// assert_eq!(gentabs(&genstring(8)), 3);
/// assert_eq!(gentabs(&genstring(9)), 3);
/// assert_eq!(gentabs(&genstring(15)), 3);
/// assert_eq!(gentabs(&genstring(16)), 2);
/// assert_eq!(gentabs(&genstring(20)), 2);
/// assert_eq!(gentabs(&genstring(24)), 1);
/// assert_eq!(gentabs(&genstring(32)), 1);
/// assert_eq!(gentabs(&genstring(100)), 1);
/// ```
pub fn gentabs(string: &str) -> usize {
    let tabcount = strwidth(string) / 8;
    if tabcount >= 4 {
        1
    } else {
        4 - tabcount
    }
}

/// Recursively create directories if missing and set permissions accordingly.
pub fn mkdir_parents<R: AsRef<Path>>(p: &R, mode: u32) -> io::Result<()> {
    DirBuilder::new()
        .mode(mode)
        .recursive(true) // directories created with same security and permissions
        .create(p.as_ref())
}

/// Counts graphemes in a given string.
///
/// ```
/// use libnewsboat::utils::graphemes_count;
///
/// assert_eq!(graphemes_count("D"), 1);
/// // len() counts bytes, not characters, but all ASCII symbols are represented by one byte in
/// // UTF-8, so len() returns 1 in this case
/// assert_eq!("D".len(), 1);
///
/// // Here's a situation where a single grapheme is represented by multiple bytes
/// assert_eq!(graphemes_count("Ж"), 1);
/// assert_eq!("Ж".len(), 2);
///
/// assert_eq!(graphemes_count("📰"), 1);
/// assert_eq!("📰".len(), 4);
/// ```
pub fn graphemes_count(input: &str) -> usize {
    UnicodeSegmentation::graphemes(input, true).count()
}

/// Extracts up to `n` first graphemes from the given string.
///
/// ```
/// use libnewsboat::utils::take_graphemes;
///
/// let input = "Привет!";
/// assert_eq!(take_graphemes(input, 1), "П");
/// assert_eq!(take_graphemes(input, 4), "Прив");
/// assert_eq!(take_graphemes(input, 6), "Привет");
/// assert_eq!(take_graphemes(input, 20), input);
/// ```
pub fn take_graphemes(input: &str, n: usize) -> String {
    UnicodeSegmentation::graphemes(input, true)
        .take(n)
        .collect::<String>()
}

/// The tag and Git commit ID the program was built from, or a pre-defined value from config.h if
/// there is no Git directory.
pub fn program_version() -> String {
    // NEWSBOAT_VERSION is set by this crate's build script, "build.rs"
    env!("NEWSBOAT_VERSION").to_string()
}

/// Newsboat's major version number.
pub fn newsboat_major_version() -> u32 {
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
        if chr == '\\' {
            prev_was_backslash = true;
            continue;
        } else if chr == '"' {
            // If the quote is escaped or we're inside backticks, do nothing
            if !prev_was_backslash && !inside_backticks {
                inside_quotes = !inside_quotes;
            }
        } else if chr == '`' {
            // If the backtick is escaped, do nothing
            if !prev_was_backslash {
                inside_backticks = !inside_backticks;
            }
        } else if chr == '#' {
            if !prev_was_backslash && !inside_quotes && !inside_backticks {
                first_pound_chr_idx = idx;
                break;
            }
        }

        // We call `continue` when we run into a backslash; here, we handle all the other
        // characters, which clearly *aren't* a backslash
        prev_was_backslash = false;
    }

    &line[0..first_pound_chr_idx]
}

#[cfg(test)]
mod tests {
    extern crate tempfile;

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
        assert_eq!(strwidth("\u{0007}"), 0);
    }

    #[test]
    fn t_strwidth_stfl() {
        assert_eq!(strwidth_stfl(""), 0);
        assert_eq!(strwidth_stfl("x<hi>x"), 3);
        assert_eq!(strwidth_stfl("x<>x"), 4);
        assert_eq!(strwidth_stfl("\u{F91F}"), 2);
        assert_eq!(strwidth_stfl("\u{0007}"), 0);
        assert_eq!(strwidth_stfl("<a"), 0); // #415
    }

    #[test]
    fn t_is_valid_podcast_type() {
        assert!(is_valid_podcast_type("audio/mpeg"));
        assert!(is_valid_podcast_type("audio/mp3"));
        assert!(is_valid_podcast_type("audio/x-mp3"));
        assert!(is_valid_podcast_type("audio/ogg"));
        assert!(is_valid_podcast_type("application/ogg"));

        assert!(!is_valid_podcast_type("image/jpeg"));
        assert!(!is_valid_podcast_type("image/png"));
        assert!(!is_valid_podcast_type("text/plain"));
        assert!(!is_valid_podcast_type("application/zip"));
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
        assert!(unescape_url(String::from("foo%20bar")).unwrap() == String::from("foo bar"));
        assert!(
            unescape_url(String::from(
                "%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D"
            ))
            .unwrap()
                == String::from("!#$&'()*+,/:;=?@[]")
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
        use self::tempfile::TempDir;
        use std::{thread, time};

        let tmp = TempDir::new().unwrap();
        let filepath = {
            let mut filepath = tmp.path().to_owned();
            filepath.push("sentry");
            filepath
        };
        assert!(!filepath.exists());

        run_command("touch", filepath.to_str().unwrap());

        thread::sleep(time::Duration::from_millis(10));

        assert!(filepath.exists());
    }

    #[test]
    fn t_run_command_doesnt_wait_for_the_command_to_finish() {
        use std::time::{Duration, Instant};

        let start = Instant::now();

        let five: &str = "5";
        run_command("sleep", five);

        let runtime = start.elapsed();

        assert!(runtime < Duration::from_secs(1));
    }

    #[test]
    fn t_run_program() {
        let input1 = "this is a multine-line\ntest string";
        assert_eq!(run_program(&["cat"], input1), input1);

        assert_eq!(
            run_program(&["echo", "-n", "hello world"], ""),
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
        use self::tempfile::TempDir;
        use std::fs;
        use std::os::unix::fs::PermissionsExt;

        let mode: u32 = 0o700;
        let tmp_dir = TempDir::new().unwrap();
        let path = tmp_dir.path().join("parent/dir");
        assert_eq!(path.exists(), false);

        let result = mkdir_parents(&path, mode);
        assert!(result.is_ok());
        assert_eq!(path.exists(), true);

        let file_type_mask = 0o7777;
        let metadata = fs::metadata(&path).unwrap();
        assert_eq!(file_type_mask & metadata.permissions().mode(), mode);

        // rerun on existing directories
        let result = mkdir_parents(&path, mode);
        assert!(result.is_ok());
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
        let expected = r#"some `other \` tricky # test` hehe"#;
        let input = expected.to_owned() + "#here goescomment";
        assert_eq!(strip_comments(&input), expected);

        // Ignores escaped # characters (\\#)
        let expected = r#"one two \# three four"#;
        let input = expected.to_owned() + "# and a comment";
        assert_eq!(strip_comments(&input), expected);
    }
}
