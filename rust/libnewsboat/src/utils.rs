extern crate rand;

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
}

