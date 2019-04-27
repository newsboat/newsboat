mod specifiers_iterator;

macro_rules! fmt {
    ( $format:expr ) => {{
        let result: &str = $format;
        result
    }};
}

#[cfg(test)]
mod tests {
    #[test]
    fn returns_first_argument_if_it_is_the_only_one() {
        let input = String::from("Hello, world!");
        assert_eq!(fmt!(&input), input);
    }
}
