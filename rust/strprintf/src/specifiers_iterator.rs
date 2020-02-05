//! An iterator over `&str`, returning substrings that contain a single format specifier (i.e.
//! percent sign followed by some more symbols, like "%.4f hello").
//!
//! **This module should only be used through `fmt!` macro.**
#![doc(hidden)]

pub struct SpecifiersIterator<'a> {
    /// The string this object iterates over.
    ///
    /// Every time the iterator returns a substring, that substring is removed from the start of
    /// the string. For example, as `SpecifiersIterator` iterates over a string "%i%o%u", the
    /// string will be shortened first to "%o%u", then to "%u", and finally to "" (empty string).
    /// After that, all subsequent calls to `next()` will return `None`.
    current_string: &'a str,
}

impl<'a> SpecifiersIterator<'a> {
    /// Create an iterator from a given string slice.
    pub fn from(input: &'a str) -> SpecifiersIterator<'a> {
        SpecifiersIterator {
            current_string: input,
        }
    }
}

/// Returns positions of the first two percent signs in the string, or None if there are less than
/// two percent signs.
fn find_percent_signs(input: &str) -> Option<(usize, usize)> {
    input.find('%').and_then(|first| {
        input[first + 1..]
            .find('%')
            .map(|second| (first, first + 1 + second))
    })
}

impl<'a> Iterator for SpecifiersIterator<'a> {
    type Item = &'a str;

    fn next(&mut self) -> Option<Self::Item> {
        let mut pair;
        let mut current_offset = 0usize;
        loop {
            pair = find_percent_signs(&self.current_string[current_offset..])
                .map(|(first, second)| (first + current_offset, second + current_offset));
            match pair {
                Some((first, second)) if second - first == 1 => {
                    // Found an escaped percent sign; continue the search after it
                    current_offset = second + 1
                }

                Some(_) => {
                    // Found a sequence of percent signs that is *not* an escaped percent sign
                    break;
                }

                // Found no percent signs at all, no point in looping anymore
                None => break,
            }
        }

        match pair {
            Some((_, second_percent_sign_pos)) => {
                let (retval, new_string) = self.current_string.split_at(second_percent_sign_pos);
                self.current_string = new_string;
                Some(retval)
            }

            None if self.current_string.is_empty() => None,

            None => {
                let retval = self.current_string;
                self.current_string = "";
                Some(&retval)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn is_immediately_exhausted_if_given_and_empty_string() {
        let mut iterator = SpecifiersIterator::from("");
        assert_eq!(iterator.next(), None);
    }

    #[test]
    fn returns_whole_original_string_if_there_are_no_specifiers() {
        let input = "Hello, world! How are you today?";
        let mut iterator = SpecifiersIterator::from(input);
        assert_eq!(iterator.next(), Some(input));
        assert_eq!(iterator.next(), None);
    }

    #[test]
    fn returns_specifiers_one_by_one_until_exhausted() {
        let input = "%i in decimal is the same as %o in octal or %x in hexadecimal";
        let mut iterator = SpecifiersIterator::from(input);
        assert_eq!(iterator.next(), Some("%i in decimal is the same as "));
        assert_eq!(iterator.next(), Some("%o in octal or "));
        assert_eq!(iterator.next(), Some("%x in hexadecimal"));
        assert_eq!(iterator.next(), None);
    }

    #[test]
    fn each_returned_string_contains_one_specifier() {
        let input = "There were %i forks";
        let mut iterator = SpecifiersIterator::from(input);
        assert_eq!(iterator.next(), Some("There were %i forks"));
        assert_eq!(iterator.next(), None);
    }

    #[test]
    fn does_not_split_escaped_percent_sign() {
        let input = "Only %.2f%% of implementations conform";
        let mut iterator = SpecifiersIterator::from(input);
        assert_eq!(iterator.next(), Some("Only %.2f"));
        assert_eq!(iterator.next(), Some("%% of implementations conform"));
        assert_eq!(iterator.next(), None);
    }

    #[test]
    fn splits_2_megabyte_string() {
        let spacer = String::from(" ").repeat(512);
        let format = {
            let mut result = spacer.clone();
            result.push_str("%i");
            result.push_str(&spacer);
            result.push_str("%i");
            result
        };
        let mut iterator = SpecifiersIterator::from(&format);
        let expected = {
            let mut result = spacer.clone();
            result.push_str("%i");
            result.push_str(&spacer);
            result
        };
        assert_eq!(iterator.next(), Some(&*expected));
        assert_eq!(iterator.next(), Some("%i"));
        assert_eq!(iterator.next(), None);
    }
}
