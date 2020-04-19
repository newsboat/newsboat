/// An entity that can be matched against a filter expression using `Matcher`.
pub trait Matchable {
    /// Returns the value of the attribute named `attr`, or `None` if there is no such attribute.
    fn attribute_value(&self, attr: &str) -> Option<String>;
}
