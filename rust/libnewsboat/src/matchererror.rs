/// Errors produced by `Matcher::matches`.
///
/// These correspond to `MatcherException` on the C++ side.
#[derive(Debug)]
pub enum MatcherError {
    /// Current matchable doesn't have an attribute named `attr`
    AttributeUnavailable { attr: String },

    /// Compiling regular expression `regex` produced an error message `errmsg`
    InvalidRegex { regex: String, errmsg: String },
}
