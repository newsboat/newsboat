macro_rules! fmt {
    ( $format:expr ) => {
        $format
    };
}

#[cfg(test)]
mod tests {
    #[test]
    fn returns_first_argument_if_it_is_the_only_one() {
        let input = "Hello, world!";
        assert_eq!(fmt!(input), input);
    }
}
