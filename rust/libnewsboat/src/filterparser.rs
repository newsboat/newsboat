//! Parses filter expressions.

/// Operators that can be used in comparisons.
#[derive(Debug, Clone, PartialEq)]
pub enum Operator {
    Equals,
    NotEquals,
    RegexMatches,
    NotRegexMatches,
    LessThan,
    GreaterThan,
    LessThanOrEquals,
    GreaterThanOrEquals,
    Between,
    Contains,
    NotContains,
}

/// Values that can be used on the right-hand side of comparisons.
#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Str(String),
    Int(i32),
    Range(i32, i32),
}

/// Parsed filter expression.
///
/// This is a tree, where nodes are logical operators (`and`, `or`), and leaves are simple
/// comparisons (`title = "hello"`, `age > 14` etc.)
#[derive(Debug, Clone, PartialEq)]
pub enum Expression {
    And(Box<Expression>, Box<Expression>),
    Or(Box<Expression>, Box<Expression>),
    Comparison {
        attribute: String,
        op: Operator,
        value: Value,
    },
}

/// Errors that may come up during parsing.
#[derive(PartialEq, Debug)]
pub enum Error<'a> {
    /// Parser finished the work, but the input string still contains some characters.
    TrailingCharacters(&'a str),

    /// Parsing error at given position.
    AtPos(usize),
}

/// Parse a string `expr` as a filter expression.
pub fn parse(expr: &str) -> Result<Expression, Error> {
    unimplemented!()
}
