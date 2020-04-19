//! Checks if given filter expression is true for a given feed or article.

use filterparser::Expression;
use matchable::Matchable;
use matchererror::MatcherError;

/// Checks if given filter expression is true for a given feed or article.
///
/// This is used for filters, query feeds, `ignore-article` commands, and even for hiding
/// already-read feeds and items.
pub struct Matcher {
    expr: Expression,

    /// Input string from which `expr` was created
    text: String,
}

impl Matcher {
    /// Prepare a `Matcher` that will check items against the filter expression provided in
    /// `input`.
    ///
    /// If `input` can't be parsed, returns a user-readable error.
    pub fn parse(input: &str) -> Result<Matcher, String> {
        unimplemented!()
    }

    /// Check if given matchable `item` matches the filter.
    pub fn matches(&self, item: &impl Matchable) -> Result<bool, MatcherError> {
        unimplemented!()
    }

    /// The filter expression from which this `Matcher` was constructed.
    pub fn get_expression(&self) -> &str {
        &self.text
    }
}
