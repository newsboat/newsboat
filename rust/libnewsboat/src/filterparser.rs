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
        // Invalid character in operator
        assert_eq!(parse("title =¯ \"foo\""), Err(Error::AtPos(7)));
        // Incorrect string quoting
        assert_eq!(parse("a = \"b"), Err(Error::AtPos(4)));
        // Non-value to the right of equality operator
        assert_eq!(parse("a = b"), Err(Error::AtPos(4)));
        // Non-existent operator
        assert_eq!(parse("a !! \"b\""), Err(Error::AtPos(2)));

        // Unbalanced parentheses
        assert_eq!(parse("((a=\"b\")))"), Err(Error::TrailingCharacters(")")));

        // Incorrect syntax for range
        assert_eq!(
            parse("AAAA between 0:15:30"),
            Err(Error::TrailingCharacters(":30"))
        );
        // No whitespace after the `and` operator
        assert_eq!(
            parse("x = 42andy=0"),
            Err(Error::TrailingCharacters("andy=0"))
        );
        assert_eq!(
            parse("x = 42 andy=0"),
            Err(Error::TrailingCharacters("andy=0"))
        );
        // Operator without arguments
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
        // C++, it terminates the whole expression and implicitly closes the string literal. When
        // calling Rust through C FFI, we'll automatically get the C behaviour since we're passing
        // the input as `const char*` without specifying its length.
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

    // Additional tests not present in C++ suite

    #[test]
    fn t_parses_simple_queries() {
        assert_eq!(
            parse("a = \"b\""),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Str("b".to_string())
            })
        );

        assert_eq!(
            parse("(a!=\"b\")"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::NotEquals,
                value: Value::Str("b".to_string())
            })
        );

        assert_eq!(
            parse("((a=~\"b\"))"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::RegexMatches,
                value: Value::Str("b".to_string())
            })
        );

        assert_eq!(
            parse("a !~ \"b\""),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::NotRegexMatches,
                value: Value::Str("b".to_string())
            })
        );

        // This is nonsensical, but it's a syntactically valid expression, so we parse it. Matcher
        // will sort it out during evaluation.
        assert_eq!(
            parse("a < \"b\""),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::LessThan,
                value: Value::Str("b".to_string())
            })
        );

        assert_eq!(
            parse("a <= \"b\""),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::LessThanOrEquals,
                value: Value::Str("b".to_string())
            })
        );

        assert_eq!(
            parse("a > \"abc\""),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::GreaterThan,
                value: Value::Str("abc".to_string())
            })
        );

        assert_eq!(
            parse("a == \"abc\""),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Str("abc".to_string())
            })
        );

        assert_eq!(
            parse("a >= 3"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::GreaterThanOrEquals,
                value: Value::Int(3)
            })
        );

        assert_eq!(
            parse("some_value between 0:-1"),
            Ok(Comparison {
                attribute: "some_value".to_string(),
                op: Operator::Between,
                value: Value::Range(0, -1)
            })
        );

        assert_eq!(
            parse("other_string between \"impossible\""),
            Ok(Comparison {
                attribute: "other_string".to_string(),
                op: Operator::Between,
                value: Value::Str("impossible".to_string())
            })
        );

        assert_eq!(
            parse("array # \"name\""),
            Ok(Comparison {
                attribute: "array".to_string(),
                op: Operator::Contains,
                value: Value::Str("name".to_string())
            })
        );

        assert_eq!(
            parse("answers !# 42"),
            Ok(Comparison {
                attribute: "answers".to_string(),
                op: Operator::NotContains,
                value: Value::Int(42)
            })
        );

        assert_eq!(
            parse("author =~ \"\\s*Doe$\""),
            Ok(Comparison {
                attribute: "author".to_string(),
                op: Operator::RegexMatches,
                value: Value::Str("\\s*Doe$".to_string())
            })
        );
    }

    #[test]
    fn t_parses_complex_queries() {
        assert_eq!(
            parse("a = \"b\" and b = \"c\" or c = \"d\"").unwrap(),
            And(
                Box::new(Comparison {
                    attribute: "a".to_string(),
                    op: Operator::Equals,
                    value: Value::Str("b".to_string())
                }),
                Box::new(Or(
                    Box::new(Comparison {
                        attribute: "b".to_string(),
                        op: Operator::Equals,
                        value: Value::Str("c".to_string())
                    }),
                    Box::new(Comparison {
                        attribute: "c".to_string(),
                        op: Operator::Equals,
                        value: Value::Str("d".to_string())
                    }),
                ))
            )
        );

        assert_eq!(
            parse("a = \"b\" or b = \"c\" and c = \"d\"").unwrap(),
            Or(
                Box::new(Comparison {
                    attribute: "a".to_string(),
                    op: Operator::Equals,
                    value: Value::Str("b".to_string())
                }),
                Box::new(And(
                    Box::new(Comparison {
                        attribute: "b".to_string(),
                        op: Operator::Equals,
                        value: Value::Str("c".to_string())
                    }),
                    Box::new(Comparison {
                        attribute: "c".to_string(),
                        op: Operator::Equals,
                        value: Value::Str("d".to_string())
                    }),
                ))
            )
        );

        assert_eq!(
            parse("(a = \"b\" or b = \"c\") and c = \"d\"").unwrap(),
            And(
                Box::new(Or(
                    Box::new(Comparison {
                        attribute: "a".to_string(),
                        op: Operator::Equals,
                        value: Value::Str("b".to_string())
                    }),
                    Box::new(Comparison {
                        attribute: "b".to_string(),
                        op: Operator::Equals,
                        value: Value::Str("c".to_string())
                    }),
                )),
                Box::new(Comparison {
                    attribute: "c".to_string(),
                    op: Operator::Equals,
                    value: Value::Str("d".to_string())
                })
            )
        );

        assert!(parse(
            "( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""
        ).is_ok());
    }

    #[test]
    fn t_only_i32_numbers_are_parsed() {
        assert_eq!(
            parse("a = 0"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(0)
            })
        );

        assert_eq!(
            parse("a = 127"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(127)
            })
        );

        assert_eq!(
            parse("a = -128"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(-128)
            })
        );

        assert_eq!(
            parse("a = 128"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(128)
            })
        );

        assert_eq!(
            parse("a = -129"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(-129)
            })
        );

        assert_eq!(
            parse("a = 255"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(255)
            })
        );

        assert_eq!(
            parse("a = 256"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(256)
            })
        );

        assert_eq!(
            parse("a = 32767"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(32767)
            })
        );

        assert_eq!(
            parse("a = -32768"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(-32768)
            })
        );

        assert_eq!(
            parse("a = 32768"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(32768)
            })
        );

        assert_eq!(
            parse("a = -32769"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(-32769)
            })
        );

        assert_eq!(
            parse("a = 65535"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(65535)
            })
        );

        assert_eq!(
            parse("a = 65536"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(65536)
            })
        );

        assert_eq!(
            parse("a = 2147483647"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(2147483647)
            })
        );

        assert_eq!(
            parse("a = -2147483648"),
            Ok(Comparison {
                attribute: "a".to_string(),
                op: Operator::Equals,
                value: Value::Int(-2147483648)
            })
        );

        assert_eq!(parse("a = 2147483648"), Err(Error::AtPos(4)));
        assert_eq!(parse("a = -2147483649"), Err(Error::AtPos(4)));
        assert_eq!(parse("abba = -9999999999999"), Err(Error::AtPos(7)));
        assert_eq!(parse("another = 98979695999999999"), Err(Error::AtPos(10)));
    }

    #[test]
    fn t_ranges_accept_negative_numbers() {
        assert_eq!(
            parse("value between -100:-1"),
            Ok(Comparison {
                attribute: "value".to_string(),
                op: Operator::Between,
                value: Value::Range(-100, -1)
            })
        );

        assert_eq!(
            parse("value between -100:100500"),
            Ok(Comparison {
                attribute: "value".to_string(),
                op: Operator::Between,
                value: Value::Range(-100, 100500)
            })
        );

        assert_eq!(
            parse("value between 123:-10"),
            Ok(Comparison {
                attribute: "value".to_string(),
                op: Operator::Between,
                value: Value::Range(123, -10)
            })
        );
    }

    proptest::proptest! {
        #[test]
        fn does_not_crash_on_any_input(ref input in "\\PC*") {
            // Result explicitly ignored because we just want to make sure this call doesn't panic.
            let _ = parse(&input);
        }

        #[test]
        fn whitespace_doesnt_affect_results_1(ref input in r#" *a *!= *"b" *"#) {
            assert_eq!(
                parse(&input),
                Ok(Comparison {
                    attribute: "a".to_string(),
                    op: Operator::NotEquals,
                    value: Value::Str("b".to_string())
                })
            );
        }

        #[test]
        fn whitespace_doesnt_affect_results_2(ref input in r#" *( *a *!= *"b" *) *"#) {
            assert_eq!(
                parse(&input),
                Ok(Comparison {
                    attribute: "a".to_string(),
                    op: Operator::NotEquals,
                    value: Value::Str("b".to_string())
                })
            );
        }

        #[test]
        fn attribute_names_can_contain_alphanumerics_underscore_dash_and_dot(ref input in r#"[-A-Za-z0-9_.]+ == 0"#) {
            assert!(
                parse(&input).is_ok(),
            );
        }
    }
}
