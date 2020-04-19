//! Parses filter expressions.

use nom::{
    branch::alt,
    bytes::complete::{escaped, is_not, tag, take, take_while, take_while1},
    character::{is_alphanumeric, is_digit},
    combinator::{complete, map, opt, peek, recognize, value},
    error::{make_error, ErrorKind, ParseError, VerboseError},
    sequence::{delimited, separated_pair, terminated, tuple},
    IResult, Offset,
};

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

fn operators<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Operator, E> {
    alt((
        value(Operator::RegexMatches, tag("=~")),
        value(Operator::Equals, alt((tag("=="), tag("=")))),
        value(Operator::NotRegexMatches, tag("!~")),
        value(Operator::NotEquals, tag("!=")),
        value(Operator::LessThanOrEquals, tag("<=")),
        value(Operator::GreaterThanOrEquals, tag(">=")),
        value(Operator::LessThan, tag("<")),
        value(Operator::GreaterThan, tag(">")),
        value(Operator::Between, tag("between")),
        value(Operator::Contains, tag("#")),
        value(Operator::NotContains, tag("!#")),
    ))(input)
}

fn quoted_string<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Value, E> {
    let empty_string = value(String::new(), tag("\"\""));
    let nonempty_string = |input| {
        let (leftovers, chr) = delimited(
            tag("\""),
            escaped(is_not("\\\""), '\\', take(1usize)),
            tag("\""),
        )(input)?;
        Ok((leftovers, String::from(chr)))
    };

    map(alt((nonempty_string, empty_string)), Value::Str)(input)
}

fn number<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, i32, E> {
    recognize(tuple((opt(tag("-")), take_while1(|c| is_digit(c as u8)))))(input).and_then(
        |(leftovers, num)| {
            match num.parse() {
                Ok(number) => Ok((leftovers, number)),
                // It shouldn't matter what ErrorKind we use here: we don't examine it anyway, and
                // only use the `input` part to figure out the location of the error.
                Err(_) => Err(nom::Err::Error(make_error(input, ErrorKind::TakeWhile1))),
            }
        },
    )
}

fn range<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Value, E> {
    separated_pair(number, tag(":"), number)(input)
        .map(|(leftovers, (a, b))| (leftovers, Value::Range(a, b)))
}

/// Skips zero or more space characters.
///
/// This is different from `nom::character::complete::space0` in that this function only skips
/// spaces (ASCII 0x20), whereas Nom's function also skips tabs.
fn space0<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, &'a str, E> {
    take_while(|c| c == ' ')(input)
}

/// Skips one or more space characters.
///
/// This is different from `nom::character::complete::space1` in that this function only skips
/// spaces (ASCII 0x20), whereas Nom's function also skips tabs.
fn space1<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, &'a str, E> {
    take_while1(|c| c == ' ')(input)
}

fn comparison<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Expression, E> {
    let attribute_name =
        take_while1(|c| is_alphanumeric(c as u8) || c == '_' || c == '-' || c == '.');

    let (input, attr) = attribute_name(input)?;
    let attribute = attr.to_string();
    let (input, _) = space0(input)?;
    let (input, op) = operators(input)?;
    let (input, _) = space0(input)?;
    let (leftovers, value) = alt((quoted_string, range, map(number, Value::Int)))(input)?;

    Ok((
        leftovers,
        Expression::Comparison {
            attribute,
            op,
            value,
        },
    ))
}

fn parens<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Expression, E> {
    let (input, _) = tag("(")(input)?;
    let (input, _) = space0(input)?;
    let (input, result) = alt((expression, parens, comparison))(input)?;
    let (input, _) = space0(input)?;
    let (leftovers, _) = tag(")")(input)?;

    Ok((leftovers, result))
}

/// Peeks to see if input is either:
/// - zero or more spaces followed by opening paren
/// - one or more spaces
///
/// This is meant to be applied after matching "and" or "or" in the input. These logical operators
/// require a space after them iff they're followed by something other than parenthesized
/// expression. For example, these are all valid expressions:
/// - "x=1and y=0"
/// - "x=1and(y=0)"
///
/// While this one is invalid:
/// - "x=1andy=0": no space between operator `and` and attribute name `y`
fn space_after_logop<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, &'a str, E> {
    let opt_space_and_paren = recognize(tuple((space0, tag("("))));
    let parser = alt((opt_space_and_paren, space1));
    peek(parser)(input)
}

fn expression<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Expression, E> {
    // `Expression`s enum variants can't be used as return values without filling in their
    // arguments, so we have to create another enum, which variants we can return and `match` on.
    // This is better than matching on bare strings returned by `tag`, as it enables us to write an
    // exhaustive `match` that the compiler can validate is exhaustive: we won't be able to add
    // a `tag` to the parser and forget to handle it.
    #[derive(Clone)]
    enum Op {
        And,
        Or,
    };

    let (input, left) = alt((parens, comparison))(input)?;
    let (input, _) = space0(input)?;
    let (input, op) = terminated(
        alt((value(Op::And, tag("and")), value(Op::Or, tag("or")))),
        space_after_logop,
    )(input)?;
    let (input, _) = space0(input)?;
    let (leftovers, right) = alt((expression, parens, comparison))(input)?;

    let op = match op {
        Op::And => Expression::And(Box::new(left), Box::new(right)),
        Op::Or => Expression::Or(Box::new(left), Box::new(right)),
    };

    Ok((leftovers, op))
}

fn parser<'a, E: ParseError<&'a str>>(input: &'a str) -> IResult<&'a str, Expression, E> {
    let parsers = alt((expression, parens, comparison));
    // Ignore leading and trailing whitespace
    let parsers = delimited(space0, parsers, space0);
    // Try to parse input. If parser says it needs more data, make that an error, since `input` is
    // all we got.
    complete(parsers)(input)
}

/// Parse a string `expr` as a filter expression.
pub fn parse(expr: &str) -> Result<Expression, Error> {
    match parser::<VerboseError<&str>>(expr) {
        Ok((leftovers, expression)) => {
            if leftovers.is_empty() {
                Ok(expression)
            } else {
                Err(Error::TrailingCharacters(&leftovers))
            }
        }
        Err(error) => {
            let handler = |e: VerboseError<&str>| -> Error {
                match e.errors.first() {
                    None => Error::AtPos(0),
                    Some((s, _kind)) => Error::AtPos(expr.offset(s)),
                }
            };

            match error {
                nom::Err::Incomplete(_) => {
                    panic!("Got nom::Err::Incomplete despite wrapping the parser into `complete()`")
                }
                nom::Err::Error(e) => Err(handler(e)),
                nom::Err::Failure(e) => Err(handler(e)),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{Expression::*, *};

    #[test]
    fn t_error_on_invalid_queries() {
        assert_eq!(parse("title =¯ \"foo\""), Err(Error::AtPos(7)));
        assert_eq!(parse("a = \"b"), Err(Error::AtPos(4)));
        assert_eq!(parse("a = b"), Err(Error::AtPos(4)));
        assert_eq!(parse("a !! \"b\""), Err(Error::AtPos(2)));

        assert_eq!(parse("((a=\"b\")))"), Err(Error::TrailingCharacters(")")));

        // From C++ Matcher suite
        assert_eq!(
            parse("AAAA between 0:15:30"),
            Err(Error::TrailingCharacters(":30"))
        );
        assert_eq!(
            parse("x = 42andy=0"),
            Err(Error::TrailingCharacters("andy=0"))
        );
        assert_eq!(
            parse("x = 42 andy=0"),
            Err(Error::TrailingCharacters("andy=0"))
        );
        assert_eq!(parse("=!"), Err(Error::AtPos(0)));
    }

    #[test]
    fn t_no_error_on_valid_queries() {
        assert!(parse("a = \"b\"").is_ok());
        assert!(parse("(a=\"b\")").is_ok());
        assert!(parse("((a=\"b\"))").is_ok());
        assert!(parse("a != \"b\"").is_ok());
        assert!(parse("a =~ \"b\"").is_ok());
        assert!(parse("a !~ \"b\"").is_ok());
        assert!(parse(
				"( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\"").is_ok());
    }

    #[test]
    fn t_both_equals_and_double_equals_are_accepted() {
        let expected = Ok(Expression::Comparison {
            attribute: "a".to_string(),
            op: Operator::Equals,
            value: Value::Str("abc".to_string()),
        });

        assert_eq!(parse("a = \"abc\""), expected);
        assert_eq!(parse("a == \"abc\""), expected);
    }

    #[test]
    fn t_filterparser_disallows_nul_byte() {
        assert!(parse("attri\0bute = 0").is_err());
        assert!(parse("attribute\0= 0").is_err());
        assert!(parse("attribute = \0").is_err());
        assert!(parse("attribute = \\\"\0\\\"").is_err());

        // Unlike the C++ implementation, Rust is OK with having NUL inside a string literal. In
        // C++, it terminates the whole expression and implicitly closes the string literal
        assert_eq!(
            parse("attribute = \"hello\0world\""),
            Ok(Expression::Comparison {
                attribute: "attribute".to_string(),
                op: Operator::Equals,
                value: Value::Str("hello\0world".to_string()),
            })
        );
    }

    #[test]
    fn t_parses_empty_string_literals() {
        let expected = Ok(Expression::Comparison {
            attribute: "title".to_string(),
            op: Operator::Equals,
            value: Value::Str(String::new()),
        });

        assert_eq!(parse("title==\"\""), expected);
    }

    #[test]
    fn t_and_operator_requires_space_or_paren_after_it() {
        assert!(parse("a=42andy=0").is_err());
        assert!(parse("(a=42)andy=0").is_err());
        assert!(parse("a=42 andy=0").is_err());

        let expected_tree = And(
            Box::new(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(42),
            }),
            Box::new(Comparison {
                attribute: "y".to_string(),
                op: Operator::Equals,
                value: Value::Int(0),
            }),
        );

        assert_eq!(parse("a=42and(y=0)"), Ok(expected_tree.clone()));
        assert_eq!(parse("(a=42)and(y=0)"), Ok(expected_tree.clone()));
        assert_eq!(parse("a=42and y=0"), Ok(expected_tree));
    }

    #[test]
    fn t_or_operator_requires_space_or_paren_after_it() {
        assert!(parse("a=42ory=0").is_err());
        assert!(parse("(a=42)ory=0").is_err());
        assert!(parse("a=42 ory=0").is_err());

        let expected_tree = Or(
            Box::new(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(42),
            }),
            Box::new(Comparison {
                attribute: "y".to_string(),
                op: Operator::Equals,
                value: Value::Int(0),
            }),
        );

        assert_eq!(parse("a=42or(y=0)"), Ok(expected_tree.clone()));
        assert_eq!(parse("(a=42)or(y=0)"), Ok(expected_tree.clone()));
        assert_eq!(parse("a=42or y=0"), Ok(expected_tree));
    }

    #[test]
    fn t_space_chars_in_filter_expr_dont_affect_parsing() {
        let expected = Comparison {
            attribute: "array".to_string(),
            op: Operator::Contains,
            value: Value::Str("bar".to_string()),
        };

        assert_eq!(parse("array # \"bar\""), Ok(expected.clone()));
        assert_eq!(parse("   array # \"bar\""), Ok(expected.clone()));
        assert_eq!(parse("array   # \"bar\""), Ok(expected.clone()));
        assert_eq!(parse("array #   \"bar\""), Ok(expected.clone()));
        assert_eq!(parse("array # \"bar\"     "), Ok(expected.clone()));
        assert_eq!(parse("array#  \"bar\"  "), Ok(expected.clone()));
        assert_eq!(
            parse("     array         #         \"bar\"      "),
            Ok(expected)
        );
    }

    #[test]
    fn t_only_space_characters_are_considered_whitespace_by_filter_parser() {
        assert_eq!(parse("attr\t= \"value\""), Err(Error::AtPos(4)));
        assert_eq!(parse("attr =\t\"value\""), Err(Error::AtPos(6)));
        assert_eq!(parse("attr\n=\t\"value\""), Err(Error::AtPos(4)));
        assert_eq!(parse("attr\u{b}=\"value\""), Err(Error::AtPos(4)));
        assert_eq!(
            parse("attr=\"value\"\r\n"),
            Err(Error::TrailingCharacters("\r\n"))
        );
    }

    #[test]
    fn t_whitespace_before_and_or_operators_is_not_required() {
        assert_eq!(
            parse("x = 42and y=0"),
            Ok(And(
                Box::new(Comparison {
                    attribute: "x".to_string(),
                    op: Operator::Equals,
                    value: Value::Int(42)
                }),
                Box::new(Comparison {
                    attribute: "y".to_string(),
                    op: Operator::Equals,
                    value: Value::Int(0)
                })
            ))
        );

        assert_eq!(
            parse("x = \"42\"and y=0"),
            Ok(And(
                Box::new(Comparison {
                    attribute: "x".to_string(),
                    op: Operator::Equals,
                    value: Value::Str("42".to_string())
                }),
                Box::new(Comparison {
                    attribute: "y".to_string(),
                    op: Operator::Equals,
                    value: Value::Int(0)
                })
            ))
        );

        assert_eq!(
            parse("x = \"42\"or y=42"),
            Ok(Or(
                Box::new(Comparison {
                    attribute: "x".to_string(),
                    op: Operator::Equals,
                    value: Value::Str("42".to_string())
                }),
                Box::new(Comparison {
                    attribute: "y".to_string(),
                    op: Operator::Equals,
                    value: Value::Int(42)
                })
            ))
        );
    }
}
