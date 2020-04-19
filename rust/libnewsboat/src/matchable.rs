/// An entity that can be matched against a filter expression using `Matcher`.
pub trait Matchable {
    /// Returns `true` if the entity has an attribute named `attr`.
    fn has_attribute(&self, attr: &str) -> bool;

    /// Returns the value of the attribute named `attr`.
    ///
    /// Value undefined if `has_attribute(attr) == false`.
    fn get_attribute(&self, attr: &str) -> String;
}
