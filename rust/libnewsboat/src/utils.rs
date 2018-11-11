extern crate rand;
use std::collections::HashSet;
use once_cell::sync::Lazy;

const COLORS: Lazy<HashSet<&'static str>> = sync_lazy! {
    let mut set = HashSet::new();
    set.insert("black");
    set.insert("red");
    set.insert("green");
    set.insert("yellow");
    set.insert("blue");
    set.insert("magenta");
    set.insert("cyan");
    set.insert("white");
    set.insert("default");
    set
};

const ATTRIBUTES: Lazy<HashSet<&'static str>> = sync_lazy!{
    let mut set = HashSet::new();
    set.insert("standout");
    set.insert("underline");
    set.insert("reverse");
    set.insert("blink");
    set.insert("dim");
    set.insert("bold");
    set.insert("protect");
    set.insert("invis");
    set.insert("default");
    set
};

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
    if COLORS.contains(color) {
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

pub fn is_valid_attribute(attr: &str) -> bool {
    ATTRIBUTES.contains(attr)
}

#[cfg(test)]
mod tests {
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
        assert_eq!(consolidate_whitespace(String::from(" Lorem \r\n\r\n\tIpsum")),
            String::from(" Lorem Ipsum"));
        assert_eq!(consolidate_whitespace(String::new()), String::new());
        assert_eq!(consolidate_whitespace(String::from("    Lorem \t\tIpsum \t ")),
            String::from("    Lorem Ipsum "));
        assert_eq!(consolidate_whitespace(String::from("   Lorem \r\n\r\n\tIpsum")),
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
}

