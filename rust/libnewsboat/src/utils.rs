pub fn replace_all(input: String, from: &str, to: &str) -> String {
    input.replace(from, to)
}

pub fn consolidate_whitespace( input: String ) -> String {
    let found = input.find( |c: char| !c.is_whitespace() );
    let mut result = String::new();

    if found.is_some() {
        let (leading,rest) = input.split_at(found.unwrap());
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
}

