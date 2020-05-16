//! A string that never grows beyond a specified length, if any.
//!
//! The length is calculated by summing the "displayed width" of all character.
//! This counts the number of "columns" the text would occupy if displayed with a monospace font.
//! For example, "abc" has length 3, but "ＡＢＣ" has length 6, even though they both contain 3 graphemes.

use crate::utils;

pub struct LimitedString {
    /// Maximum length of this string, counted by "displayed width".
    max_length: Option<usize>,
    /// The contents of the limited string.
    content: String,
}

impl LimitedString {
    /// Creates a new `LimitedString`, with an optional length limit.
    pub fn new(max_length: Option<usize>) -> LimitedString {
        LimitedString {
            max_length,
            content: String::new(),
        }
    }

    /// Returns the "displayed width" of the string
    pub fn length(&self) -> usize {
        utils::strwidth(&self.content)
    }

    /// Adds given character to the end of the string, or does nothing if the string length reached
    /// the limit.
    pub fn push(&mut self, c: char) {
        if let Some(limit) = self.max_length {
            if self.length() + utils::strwidth(&c.to_string()) <= limit {
                self.content.push(c);
            }
        } else {
            self.content.push(c);
        }
    }

    /// Adds given string to the end of the string, or does nothing if the string length reached
    /// the limit.
    pub fn push_str(&mut self, s: &str) {
        if let Some(limit) = self.max_length {
            let remaining_length = limit - self.length();
            self.content
                .push_str(&utils::substr_with_width(s, remaining_length));
        } else {
            self.content.push_str(s);
        }
    }

    /// Consumes LimitedString and turns it into an ordinary String.
    pub fn into_string(self) -> String {
        self.content
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_default_length_is_zero() {
        assert_eq!(LimitedString::new(None).length(), 0);
    }

    #[test]
    fn t_length_never_exceeds_the_limit() {
        let limit = 15usize;
        let mut s = LimitedString::new(Some(limit));

        s.push_str("Привет, мир!");
        assert!(s.length() < limit);

        s.push_str("!!");
        assert!(s.length() < limit);

        s.push_str(" Ну как, всё нормально?");
        assert_eq!(s.length(), limit);
    }

    #[test]
    fn t_into_string_returns_a_string_of_the_same_length_no_limit_case() {
        let mut s = LimitedString::new(None);

        s.push_str("Hello, world! ☺");
        let expected_length = 15usize;
        assert_eq!(s.length(), expected_length);

        let exported_string = s.into_string();
        assert_eq!(utils::strwidth(&exported_string), expected_length);
    }

    #[test]
    fn t_into_string_returns_a_string_of_the_same_length_limited_case() {
        let limit = 12usize;
        let mut s = LimitedString::new(Some(limit));

        s.push_str("Hello☺, world!");
        assert_eq!(s.length(), limit);

        let exported_string = s.into_string();
        assert_eq!(utils::strwidth(&exported_string), limit);
    }

    #[test]
    fn t_push_cannot_make_the_string_exceed_the_limit() {
        let limit = 3usize;
        let mut s = LimitedString::new(Some(limit));

        // This is "konnitiwa", "good afternoon" in Japanese.
        s.push('こ');
        assert!(s.length() < limit);
        s.push('ん');
        assert!(s.length() < limit);
        s.push('-');
        assert_eq!(s.length(), limit);
        s.push('に');
        assert_eq!(s.length(), limit);
        s.push('ち');
        assert_eq!(s.length(), limit);
        s.push('は');
        assert_eq!(s.length(), limit);
    }

    proptest::proptest! {
        #[test]
        fn length_never_exceeds_the_limit_one_string(
            limit in 1usize..1024,
            ref input in "\\PC*")
        {
            let mut s = LimitedString::new(Some(limit));
            s.push_str(&input);
            assert!(s.length() <= limit);
        }

        #[test]
        fn length_never_exceeds_the_limit_two_strings(
            limit in 1usize..1024,
            ref input1 in "\\PC*",
            ref input2 in "\\PC*")
        {
            let mut s = LimitedString::new(Some(limit));
            s.push_str(&input1);
            assert!(s.length() <= limit);
            s.push_str(&input2);
            assert!(s.length() <= limit);
        }

        #[test]
        fn length_never_exceeds_the_limit_three_strings(
            limit in 1usize..1024,
            ref input1 in "\\PC*",
            ref input2 in "\\PC*",
            ref input3 in "\\PC*")
        {
            let mut s = LimitedString::new(Some(limit));
            s.push_str(&input1);
            assert!(s.length() <= limit);
            s.push_str(&input2);
            assert!(s.length() <= limit);
            s.push_str(&input3);
            assert!(s.length() <= limit);
        }

        #[test]
        fn push_cannot_make_the_string_exceed_the_limit(
            limit in 1usize..1024,
            ref input in "\\PC*")
        {
            let mut s = LimitedString::new(Some(limit));

            // Ordinary for loop doesn't compile here, ostensibly because Proptest tries to
            // interpret it.
            let _ = input
                .chars()
                .map(|chr| { s.push(chr); assert!(s.length() <= limit); })
                .collect::<Vec<_>>();
        }
    }
}
