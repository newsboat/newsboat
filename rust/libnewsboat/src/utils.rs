extern crate dirs;
extern crate rand;
extern crate regex;
extern crate unicode_segmentation;
extern crate unicode_width;
extern crate url;

use self::regex::Regex;
use self::unicode_segmentation::UnicodeSegmentation;
use self::unicode_width::UnicodeWidthStr;
use self::url::percent_encoding::*;
use self::url::Url;
use logger::{self, Level};
use std::io::Write;
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
        .map(|url| url.as_str())
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
    return file_path;
}

pub fn resolve_relative(reference: &Path, path: &Path) -> PathBuf {
    if path.is_relative() {
        // Will only ever panic if reference is `/`, which shouldn't be the case as reference is
        // always a file path
        return reference.parent().unwrap().join(path);
    }
    return path.to_path_buf();
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
            .map(|url| url.as_str())
            .unwrap_or(url)
            .to_owned()
    } else {
        url.into()
    }
}

pub fn get_default_browser() -> String {
    use std::env;
    match env::var("BROWSER") {
        Ok(val) => return val,
        Err(_) => return String::from("lynx"),
    }
}

pub fn trim(rs_str: String) -> String {
    rs_str.trim().to_string()
}

pub fn trim_end(rs_str: String) -> String {
    let x: &[_] = &['\n', '\r'];
    rs_str.trim_right_matches(x).to_string()
}

pub fn quote(input: String) -> String {
    let mut input = input.replace("\"", "\\\"");
    input.insert(0, '"');
    input.push('"');
    input
}

pub fn quote_if_necessary(input: String) -> String {
    if input.find(" ") != None {
        quote(input)
    } else {
        input
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
        return true;
    }
    if color.starts_with("color0") {
        return color == "color0";
    }
    if color.starts_with("color") {
        let num_part = &color[5..];
        match num_part.parse::<u8>() {
            Ok(_) => return true,
            _ => return false,
        }
    }
    false
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
    let control = rs_str.chars().fold(true, |acc, x| acc & !x.is_control());

    if control {
        return UnicodeWidthStr::width(rs_str);
    } else {
        return rs_str.len();
    }
}

pub fn strwidth_stfl(rs_str: &str) -> usize {
    let mut reduce: usize = 0;
    let mut chars = rs_str.chars().peekable();
    loop {
        match chars.next() {
            Some('<') => {
                if chars.peek() != Some(&'>') {
                    reduce += 3;
                }
            }
            Some(_) => continue,
            None => break,
        }
    }

    let width = strwidth(rs_str);
    if width < reduce {
        0
    } else {
        width - reduce
    }
}

pub fn is_valid_podcast_type(mimetype: &str) -> bool {
    let re = Regex::new(r"(audio|video)/.*").unwrap();
    let matches = re.is_match(mimetype);

    let acceptable = ["application/ogg"];
    let found = acceptable.contains(&mimetype);

    matches || found
}

pub fn unescape_url(rs_str: String) -> Option<String> {
    let result = percent_decode(rs_str.as_bytes());
    let result = result.decode_utf8();
    if result.is_err() {
        return None;
    }

    Some(result.unwrap().replace("\0", ""))
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
    let mut result = rs_str.trim_right_matches('/');

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
        .trim_right_matches(".html")
        .trim_right_matches(".php");
    result = result
        .trim_right_matches(".aspx")
        .trim_right_matches(".htm");

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
        assert!(strwidth("") == 0);
        assert!(strwidth("xx") == 2);
        assert!(strwidth("\u{F91F}") == 2);
        assert!(strwidth("\u{0007}") == 1);
    }

    #[test]
    fn t_strwidth_stfl() {
        assert!(strwidth_stfl("") == 0);
        assert!(strwidth_stfl("x<hi>x") == 3);
        assert!(strwidth_stfl("x<>x") == 4);
        assert!(strwidth_stfl("\u{F91F}") == 2);
        assert!(strwidth_stfl("\u{0007}") == 1);
        assert!(strwidth_stfl("<a") == 0); // #415
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
        assert!(make_title(input) == String::from("Item"));

        input = String::from("http://example.com/This-is-the-title");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/This_is_the_title");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.php");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.html");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.htm");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/This_is-the_title.aspx");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/this-is-the-title");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/items/misc/this-is-the-title");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/item/");
        assert!(make_title(input) == String::from("Item"));

        input = String::from("http://example.com/item/////////////");
        assert!(make_title(input) == String::from("Item"));

        input = String::from("blahscheme://example.com/this-is-the-title");
        assert!(make_title(input) == String::from("This is the title"));

        input = String::from("http://example.com/story/aug/title-with-dashes?a=b");
        assert!(make_title(input) == String::from("Title with dashes"));

        input = String::from("http://example.com/title-with-dashes?a=b&x=y&utf8=✓");
        assert!(make_title(input) == String::from("Title with dashes"));

        input = String::from("https://example.com/It%27s%202017%21");
        assert!(make_title(input) == String::from("It's 2017!"));

        input = String::from("https://example.com/?format=rss");
        assert!(make_title(input) == String::from(""));

        assert!(make_title(String::from("")) == String::from(""));
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
}
