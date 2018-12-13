extern crate rand;
extern crate regex;
extern crate url;
extern crate dirs;

use logger::{self, Level};
use self::regex::Regex;
use self::url::{Url};
use self::url::percent_encoding::*;
use std::process::{Command, Stdio};

pub fn replace_all(input: String, from: &str, to: &str) -> String {
    input.replace(from, to)
}

pub fn consolidate_whitespace( input: String ) -> String {
    let found = input.find( |c: char| !c.is_whitespace() );
    let mut result = String::new();

    if let Some(found) = found {
        let (leading,rest) = input.split_at(found);
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
    let mut file_path:String = path;
    let home_path = dirs::home_dir();

    if let Some(home_path) = home_path {
        let home_path_string = home_path.to_string_lossy().into_owned();

        if file_path == "~" {
            file_path = home_path_string;
        } else {
            let tmp_file_path = file_path.clone();

            if tmp_file_path.len() > 1 {

                let (tilde,remaining) = tmp_file_path.split_at(2);

                if tilde == "~/" {
                    file_path = home_path_string + "/" + remaining;
                }
            }
        }
    }
    return file_path;
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
        Url::parse(url).map(|mut url| {
            if url.username() != "" || url.password().is_some()  {
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
    let x: &[_] = &['\n','\r'];
    rs_str.trim_right_matches(x).to_string()
}

pub fn get_random_value(max: u32) -> u32 {
   rand::random::<u32>() % max
}

pub fn is_valid_color(color: &str) -> bool {
    const COLORS: [&str; 9] = [
        "black",
        "red",
        "green",
        "yellow",
        "blue",
        "magenta",
        "cyan",
        "white",
        "default",
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

pub fn is_valid_attribute(attribute:  &str) -> bool {
    const VALID_ATTRIBUTES: [&str; 9]  = [
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
            return None
    }

    Some(result.unwrap().replace("\0",""))
}

pub fn get_command_output(cmd: &str) -> String {
    let cmd = Command::new("sh").arg("-c").arg(cmd).output();
    // from_utf8_lossy will convert any bad bytes to U+FFFD
    cmd.map(|cmd| String::from_utf8_lossy(&cmd.stdout).into_owned())
        .unwrap_or_else(|_| String::from(""))
}

pub fn run_command(cmd: &str, param: &str) {
    let child = Command::new(cmd)
        .arg(param)
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn();
    if let Err(error) = child {
        log!(Level::Debug,
             &format!("utils::run_command: spawning a child for \"{}\" failed: {}", cmd, error));
    }

    /* We deliberately *don't* wait for the child to finish. */
}

#[cfg(test)]
mod tests {
    extern crate tempfile;

    use super::*;

    #[test]
    fn t_replace_all() {
        assert_eq!(
            replace_all(String::from("aaa"), "a", "b"),
            String::from("bbb"));
        assert_eq!(
            replace_all(String::from("aaa"), "aa", "ba"),
            String::from("baa"));
        assert_eq!(
            replace_all(String::from("aaaaaa"), "aa", "ba"),
            String::from("bababa"));
        assert_eq!(
            replace_all(String::new(), "a", "b"),
            String::new());

        let input = String::from("aaaa");
        assert_eq!(replace_all(input.clone(), "b", "c"), input);

        assert_eq!(
            replace_all(String::from("this is a normal test text"), " t", " T"),
            String::from("this is a normal Test Text"));

        assert_eq!(
            replace_all(String::from("o o o"), "o", "<o>"),
            String::from("<o> <o> <o>"));
    }

    #[test]
    fn t_consolidate_whitespace() {
        assert_eq!(
            consolidate_whitespace(String::from("LoremIpsum")),
            String::from("LoremIpsum"));
        assert_eq!(
            consolidate_whitespace(String::from("Lorem Ipsum")),
            String::from("Lorem Ipsum"));
        assert_eq!(
            consolidate_whitespace(String::from(" Lorem \t\tIpsum \t ")),
            String::from(" Lorem Ipsum "));
        assert_eq!(
            consolidate_whitespace(String::from(" Lorem \r\n\r\n\tIpsum")),
            String::from(" Lorem Ipsum"));
        assert_eq!(consolidate_whitespace(String::new()), String::new());
        assert_eq!(
            consolidate_whitespace(String::from("    Lorem \t\tIpsum \t ")),
            String::from("    Lorem Ipsum "));
        assert_eq!(
            consolidate_whitespace(String::from("   Lorem \r\n\r\n\tIpsum")),
            String::from("   Lorem Ipsum"));
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
            "black",
            "red",
            "green",
            "yellow",
            "blue",
            "magenta",
            "cyan",
            "white",
            "default",
            "color0",
            "color163",
        ];

        for color in &valid {
            assert!(is_valid_color(color));
        }
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
        let invalid = [
            "foo",
            "bar",
            "baz",
            "quux",
        ];
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
        assert!(unescape_url(String::from("foo%20bar")).unwrap() ==
                String::from("foo bar"));
        assert!(unescape_url(
                String::from("%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D")).unwrap() ==
            String::from("!#$&'()*+,/:;=?@[]"));
    }

    #[test]
    fn t_get_command_output() {
        assert_eq!(get_command_output("ls /dev/null"), "/dev/null\n".to_string());
        assert_eq!(get_command_output("a-program-that-is-guaranteed-to-not-exists"), "".to_string());
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

        thread::sleep(time::Duration::from_millis(1));

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
}

