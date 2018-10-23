pub fn replace_all(input: String, from: &str, to: &str) -> String {
    input.replace(from, to)
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
}
