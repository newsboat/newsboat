//! Safe wrapper for [POSIX regular expressions API][regex-h] (provided by libc on POSIX-compliant OSes).
//!
//! [regex-h]: https://pubs.opengroup.org/onlinepubs/9699919799.2008edition/basedefs/regex.h.html#tag_13_37
//!
//! ```
//! use regex_rs::*;
//!
//! let pattern = "This( often)? repeats time and again(, and again)*\\.";
//! let compilation_flags = CompFlags::EXTENDED;
//! let regex = Regex::new(pattern, compilation_flags)
//!     .expect("Failed to compile pattern as POSIX extended regular expression");
//!
//! let input = "This repeats time and again, and again, and again.";
//! // We're only interested in the first match, i.e. the part of text
//! // that's matched by the whole regex
//! let max_matches = 1;
//! let match_flags = MatchFlags::empty();
//! let matches = regex
//!     .matches(input, max_matches, match_flags)
//!     .expect("Error matching input against regex");
//!
//! // Found a match
//! assert_eq!(matches.len(), 1);
//!
//! // Match spans from the beginning to the end of the input
//! assert_eq!(matches[0].start_pos, 0);
//! // `end_pos` holds one-past-the-end index
//! assert_eq!(matches[0].end_pos, input.len());
//! ```

// This lint is nitpicky, I don't think it's really important how the literals are written.
#![allow(clippy::unreadable_literal)]

use bitflags::bitflags;
use gettextrs::gettext;
use libc::{regcomp, regerror, regex_t, regexec, regfree, regmatch_t};
use std::ffi::{CString, OsString};
use std::mem;
use std::os::unix::ffi::OsStringExt;
use std::ptr;
use strprintf::fmt;

/// POSIX regular expression.
pub struct Regex {
    /// Compiled POSIX regular expression.
    regex: regex_t,
}

bitflags! {
    /// Compilation flags.
    ///
    /// These affect what features are available inside the regex, and also how it's matched
    /// against the input string.
    pub struct CompFlags: i32 {
        /// Use Extended Regular Expressions.
        ///
        /// POSIX calls this `REG_EXTENDED`.
        const EXTENDED = libc::REG_EXTENDED;

        /// Ignore case when matching.
        ///
        /// POSIX calls this `REG_ICASE`.
        const IGNORE_CASE = libc::REG_ICASE;

        /// Report only success or fail of the compilation.
        ///
        /// POSIX calls this `REG_NOSUB`.
        const NO_SUB = libc::REG_NOSUB;

        /// Give special meaning to newline characters.
        ///
        /// POSIX calls this `REG_NEWLINE`.
        ///
        /// Without this flag, newlines match themselves.
        ///
        /// With this flag, newlines match themselves except:
        ///
        /// 1. newline is not matched by `.` outside of bracket expressions or by any form of
        ///    non-matching lists;
        ///
        /// 2. beginning-of-line (`^`) matches zero-width string right after newline, regardless of
        ///    `CompFlag::NOTBOL`;
        ///
        /// 3. end-of-line (`$`) matches zero-width string right before a newline, regardless of
        ///    `CompFlags::NOTEOL`.
        const NEWLINE = libc::REG_NEWLINE;
    }
}

bitflags! {
    /// Matching flags.
    ///
    /// These affect how regex is matched against the input string.
    pub struct MatchFlags: i32 {
        /// The circumflex character (`^`), when taken as a special character, does not match the
        /// beginning of string.
        const NOTBOL = libc::REG_NOTBOL;

        /// The dollar-sign (`$`), when taken as a special character, does not match the end of
        /// string.
        const NOTEOL = libc::REG_NOTEOL;
    }
}

/// Start and end positions of a matched substring.
pub struct Match {
    /// Start position (counting from zero).
    pub start_pos: usize,

    /// One-past-end position (counting from zero).
    pub end_pos: usize,
}

/// A wrapper around `libc::regerror()`.
unsafe fn regex_error_to_str(errcode: libc::c_int, regex: &regex_t) -> Option<String> {
    unsafe {
        // Find out the size of the buffer needed to hold the error message
        let errmsg_length = regerror(errcode, regex, ptr::null_mut(), 0);

        // Allocate the buffer and get the message.
        let mut errmsg: Vec<u8> = vec![0; errmsg_length];
        // Casting `*mut u8` to `*mut c_char` should be safe since C doesn't really care:
        // it can store any ASCII symbol in a `char`, disregarding signedness.
        regerror(
            errcode,
            regex,
            errmsg.as_mut_ptr() as *mut std::os::raw::c_char,
            errmsg_length,
        );

        // Drop the trailing NUL byte that C uses to terminate strings
        errmsg.pop();

        OsString::from_vec(errmsg).into_string().ok()
    }
}

impl Regex {
    /// Compiles pattern as a regular expression.
    ///
    /// By default, pattern is assumed to be a basic regular expression. To interpret it as an
    /// extended regular expression, add `CompFlags::EXTENDED` to the `flags`. See also other
    /// `CompFlags` values to control some other aspects of the regex.
    ///
    /// # Returns
    ///
    /// Compiled regex or an error message.
    pub fn new(pattern: &str, flags: CompFlags) -> Result<Regex, String> {
        let pattern = CString::new(pattern)
            .map_err(|_| String::from("Regular expression contains NUL byte"))?;

        unsafe {
            let mut regex: regex_t = mem::zeroed();
            let errcode = regcomp(&mut regex, pattern.into_raw(), flags.bits());

            if errcode == 0 {
                Ok(Regex { regex })
            } else {
                match regex_error_to_str(errcode, &regex) {
                    Some(regcomp_errmsg) => {
                        let msg = fmt!(&gettext("regcomp returned code %i"), errcode);
                        let msg = format!("{msg}: {regcomp_errmsg}");
                        Err(msg)
                    }

                    None => Err(fmt!(&gettext("regcomp returned code %i"), errcode)),
                }
            }
        }
    }

    /// Matches input string against regex, looking for up to `max_matches` matches.
    ///
    /// Regexes can contain parenthesized subexpressions. This method will return up to
    /// `max_matches`-1 of those. First match is reserved for the text that the whole regex
    /// matched.
    ///
    /// `flags` dictate how matching is performed. See `MatchFlags` for details.
    ///
    /// # Returns
    ///
    /// - `Ok` with an empty vector if no match found, or if `max_matches` is 0 and there were no
    ///   errors.
    /// - `Ok` with a non-empty vector if `max_matches` was non-zero and a match was found. First
    ///   element of the vector is the text that regex as a whole matched. The rest of the elements
    ///   are pieces of text that were matched by parenthesized subexpressions.
    /// - `Err` with an error message.
    pub fn matches(
        &self,
        input: &str,
        max_matches: usize,
        flags: MatchFlags,
    ) -> Result<Vec<Match>, String> {
        let input =
            CString::new(input).map_err(|_| String::from("Input string contains NUL byte"))?;

        let mut pmatch: Vec<regmatch_t>;

        let errcode = unsafe {
            pmatch = vec![mem::zeroed(); max_matches];

            regexec(
                &self.regex,
                input.into_raw(),
                max_matches as libc::size_t,
                pmatch.as_mut_ptr(),
                flags.bits(),
            )
        };

        match errcode {
            0 => {
                // Success. Let's copy results
                let mut matches: Vec<Match> = Vec::new();

                for m in pmatch {
                    if m.rm_so < 0 || m.rm_eo < 0 {
                        // Since `max_matches` can be bigger than the number of parenthesized
                        // blocks in the regex, it's possible that some of the `pmatch` values are
                        // empty. We exit the loop after detecting first such value.
                        break;
                    }

                    // It's safe to cast i32 to usize here:
                    // - we already checked that the values aren't negative
                    // - usize's upper bound is higher than i32's
                    matches.push(Match {
                        start_pos: m.rm_so as usize,
                        end_pos: m.rm_eo as usize,
                    });
                }

                Ok(matches)
            }

            libc::REG_NOMATCH => {
                // Matching went okay, but nothing found
                Ok(Vec::new())
            }

            // POSIX only specifies two return codes for regexec(), but implementations are free to
            // extend that.
            _ => unsafe {
                match regex_error_to_str(errcode, &self.regex) {
                    Some(regexec_errmsg) => {
                        let msg = fmt!(&gettext("regexec returned code %i"), errcode);
                        let msg = format!("{msg}: {regexec_errmsg}");
                        Err(msg)
                    }
                    None => Err(fmt!(&gettext("regexec returned code %i"), errcode)),
                }
            },
        }
    }
}

impl Drop for Regex {
    fn drop(&mut self) {
        unsafe {
            regfree(&mut self.regex);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn matches_basic_posix_regular_expression() {
        let regex = Regex::new("abc+", CompFlags::empty()).unwrap();
        let matches = regex.matches("abc+others", 1, MatchFlags::empty()).unwrap();

        assert_eq!(matches.len(), 1);

        // Match spans from the start of the input until the 4th character
        assert_eq!(matches[0].start_pos, 0);
        assert_eq!(matches[0].end_pos, 4); // one-past-last offset
    }

    #[test]
    fn matches_extended_posix_regular_expression() {
        let regex = Regex::new("aBc+", CompFlags::EXTENDED | CompFlags::IGNORE_CASE).unwrap();
        let matches = regex
            .matches("AbCcCcCC and others", 1, MatchFlags::empty())
            .unwrap();

        assert_eq!(matches.len(), 1);

        // Match spans from the start of the input until the 4th character
        assert_eq!(matches[0].start_pos, 0);
        assert_eq!(matches[0].end_pos, 8); // one-past-last offset
    }

    #[test]
    fn returns_empty_when_regex_valid_but_no_match() {
        let regex = Regex::new("abc", CompFlags::empty()).unwrap();
        let matches = regex.matches("cba", 1, MatchFlags::empty()).unwrap();

        assert_eq!(matches.len(), 0);
    }

    #[test]
    fn returns_no_more_results_than_max_matches() {
        let regex = Regex::new("(a)(b)(c)", CompFlags::EXTENDED).unwrap();
        let max_matches = 2;
        let matches = regex
            .matches("abc", max_matches, MatchFlags::empty())
            .unwrap();

        assert_eq!(matches.len(), 2);

        assert_eq!(matches[0].start_pos, 0);
        assert_eq!(matches[0].end_pos, 3);
        assert_eq!(matches[1].start_pos, 0);
        assert_eq!(matches[1].end_pos, 1);
    }

    #[test]
    fn returns_no_more_results_than_available() {
        let regex = Regex::new("abc", CompFlags::EXTENDED).unwrap();
        let max_matches = 10;
        let matches = regex
            .matches("abc", max_matches, MatchFlags::empty())
            .unwrap();

        assert_eq!(matches.len(), 1);

        assert_eq!(matches[0].start_pos, 0);
        assert_eq!(matches[0].end_pos, 3);
    }

    #[test]
    fn new_returns_error_on_invalid_regex() {
        let result = Regex::new("(abc", CompFlags::EXTENDED);

        assert!(result.is_err());
        if let Err(msg) = result {
            // There should be at least an error code, so string can't possibly be empty
            assert!(!msg.is_empty());

            // The message shouldn't contain a C string terminator (NUL) at the end
            assert!(!msg.ends_with('\0'));
        }
    }
}
