extern crate nom;

use std::io::{self, Read, BufRead};
use nom::*;
use nom::types::CompleteStr;

#[derive(Debug, Clone)]
pub enum Operation {
    Equals,
    NotEquals,
    RegEquals,
    NotRegEquals,
    LessThan,
    GreaterThan,
    LessThanOrEquals,
    GreaterThanOrEquals,
    Between,
    Contains,
    NotContains,
}

#[derive(Debug, Clone)]
pub enum Value {
    Str(String),
    Int(i32),
    Range(i32, i32)
}

#[derive(Debug, Clone)]
pub struct Comparison {
    pub attribute: String,
    pub op: Operation,
    pub value: Value
}

#[derive(Debug, Clone)]
pub enum Expression {
    And(Box<Expression>, Box<Expression>),
    Or(Box<Expression>, Box<Expression>),
    Comparison(Comparison)
}

named!(operators<CompleteStr, Operation>,
    do_parse!(
        tag: alt!(
            map!(tag!("=="), |_| Operation::Equals) |
            map!(tag!("!="), |_| Operation::NotEquals) |
            map!(tag!("=~"), |_| Operation::RegEquals) |
            map!(tag!("=") , |_| Operation::Equals) |
            map!(tag!("!~"), |_| Operation::NotRegEquals) |
            map!(tag!("<="), |_| Operation::LessThanOrEquals) |
            map!(tag!(">="), |_| Operation::GreaterThanOrEquals) |
            map!(tag!("<") , |_| Operation::LessThan) |
            map!(tag!(">") , |_| Operation::GreaterThan) |
            map!(tag!("between"), |_| Operation::Between) |
            map!(tag!("#"), |_| Operation::Contains) |
            map!(tag!("!#"), |_| Operation::NotContains)
        ) >>
        (tag)
    )
);

fn quoted_string(input: CompleteStr) -> IResult<CompleteStr, Value>{
    do_parse!(input,
        tag!("\"")
        >> value: escaped_transform!(
                    is_not!("\\\""),
                    '\\',
                    alt!(
                        tag!("\\") => { |_| &"\\"[..] } |
                        tag!("\"") => { |_| &"\""[..] }
                    )
            )
        >> tag!("\"")
        >> (value)
    ).map(|result| {
        (result.0, Value::Str(result.1))
    })
}

fn number(input: CompleteStr) -> IResult<CompleteStr, Value> {
    do_parse!(input,
        num: recognize!(
            do_parse!(
                opt!(tag!("-")) >>
                take_while1!(|c| is_digit(c as u8)) >>
                ()
            )
        ) >> (num)
    ).map(|result| {
        let tuple = result.1;

        let number: i32 = tuple.0.parse().unwrap();
        (result.0, Value::Int(number))
    })
}

fn range(input: CompleteStr) -> IResult<CompleteStr, Value> {
    do_parse!(input,
        a: number >>
        tag!(":") >>
        b: number >>
        (a, b)
    ).map(|result| {
        if let (Value::Int(a), Value::Int(b)) = result.1 {
            (result.0, Value::Range(a, b))
        } else {
            panic!("This should not happen");
        }
    })
}

fn condition(input: CompleteStr) -> IResult<CompleteStr, Expression>{
    do_parse!(
        input,
        attr: ws!(take_while!(|c| is_alphanumeric(c as u8) || c == '_'))
        >> op: ws!(operators)
        >> value: ws!(alt!(quoted_string | range | number))
        >> (attr, op, value)
    ).map(|result| {
        let tuple = result.1;
        let attribute = tuple.0.to_string();
        let op = tuple.1;
        let value: Value = tuple.2;

        (
            result.0,
            Expression::Comparison(
                Comparison {
                    attribute, op, value
                }
            )
        )
    })
}

fn parens(input: CompleteStr) -> IResult<CompleteStr, Expression> {
    do_parse!(
        input,
        ws!(tag!("("))
        >> ret: ws!(alt!(expression | parens | condition))
        >> ws!(tag!(")"))
        >> (ret)
    ).map(|result| {
        let ret = result.1;
        (result.0, ret)
    })
}

fn expression(input: CompleteStr) -> IResult<CompleteStr, Expression> {
    do_parse!(
        input,
        left: alt!(parens | condition) >>
        op: ws!(alt!(tag!("and") | tag!("or"))) >>
        right: alt!(expression | parens | condition) >>
        (left, op.to_string(), right)
    ).map(|result|{
        let (left, op, right) = result.1;

        let op = match op.as_ref() {
            "and" => Expression::And(Box::new(left), Box::new(right)),
            "or" => Expression::Or(Box::new(left), Box::new(right)),
            _ => panic!("Invalid expression"),
        };

        (result.0, op)
    })
}

pub fn parse(input: &str) -> Result<Expression, &str> {
    let result = do_parse!(CompleteStr(input),
        parse: alt!(expression | parens | condition) >>
        (parse)
    );
    match result {
        Ok(expr) => {
            if (expr.0.is_empty()) {
                Ok(expr.1)
            } else {
                Err("Trailing characters")
            }
        },
        Err(result) => {
            Err("Error parsing expression")
        }
    }
}

mod tests {
    use super::*;
    use super::Expression::*;
    use super::Operation::*;

    use std::mem;

    impl PartialEq<Expression> for Expression {
        fn eq(&self, other: &Expression) -> bool {
            let a = self;
            let b = other;
            mem::discriminant(a) == mem::discriminant(b)
        }
    }

    #[test]
    fn t_error_on_invalid_queries() {
        assert!(parse("title =¯ \"foo\"").is_err());
        assert!(parse("a = \"b").is_err());
        assert!(parse("a = b").is_err());
        //assert!(parse("((a=\"b\")))").is_err());
        assert!(parse("a !! \"b\"").is_err());
    }

    #[test]
    fn t_ok_on_valid_queries() {
        assert!(parse("a = \"b\"").is_ok());
        assert!(parse("(a=\"b\")").is_ok());
        assert!(parse("((a=\"b\"))").is_ok());

        assert!(parse("a != \"b\"").is_ok());
        assert!(parse("a =~ \"b\"").is_ok());
        assert!(parse("a !~ \"b\"").is_ok());

        assert!(parse(
            "( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""
        ).is_ok());

        assert!(parse("a = \"abc\"").is_ok());
        assert!(parse("a == \"abc\"").is_ok());
    }

    #[test]
    fn t_test_syntax_trees() {
        assert_eq!(parse("a = \"b\" and b = \"c\" or c = \"d\"").unwrap(),
            And(
                Box::new(Comparison(super::Comparison {
                    attribute: "a".to_string(),
                    op: Operation::Equals,
                    value: Value::Str("b".to_string())
                })),
                Box::new(Or(
                    Box::new(Comparison(super::Comparison {
                        attribute: "b".to_string(),
                        op: Operation::Equals,
                        value: Value::Str("c".to_string())
                    })),
                    Box::new(Comparison(super::Comparison {
                        attribute: "c".to_string(),
                        op: Operation::Equals,
                        value: Value::Str("d".to_string())
                    })),
                ))
            )
        );
        assert_eq!(parse("a = \"b\" or b = \"c\" and c = \"d\"").unwrap(),
            Or(
                Box::new(Comparison(super::Comparison {
                    attribute: "a".to_string(),
                    op: Operation::Equals,
                    value: Value::Str("b".to_string())
                })),
                Box::new(And(
                    Box::new(Comparison(super::Comparison {
                        attribute: "b".to_string(),
                        op: Operation::Equals,
                        value: Value::Str("c".to_string())
                    })),
                    Box::new(Comparison(super::Comparison {
                        attribute: "c".to_string(),
                        op: Operation::Equals,
                        value: Value::Str("d".to_string())
                    })),
                ))
            )
        );
        assert_eq!(parse("(a = \"b\" or b = \"c\") and c = \"d\"").unwrap(),
            And(
                Box::new(Or(
                    Box::new(Comparison(super::Comparison {
                        attribute: "a".to_string(),
                        op: Operation::Equals,
                        value: Value::Str("b".to_string())
                    })),
                    Box::new(Comparison(super::Comparison {
                        attribute: "b".to_string(),
                        op: Operation::Equals,
                        value: Value::Str("c".to_string())
                    })),
                )),
                Box::new(Comparison(super::Comparison {
                    attribute: "c".to_string(),
                    op: Operation::Equals,
                    value: Value::Str("d".to_string())
                }))
            )
        );
    }
}
