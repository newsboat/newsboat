extern crate nom;

use std::io::{self, Read, BufRead};
use nom::*;
use nom::types::CompleteStr;

#[derive(Debug, Clone)]
pub enum Value {
    Str(String),
    Int(i32),
    Range(i32, i32)
}

#[derive(Debug, Clone)]
pub struct Condition {
    pub attribute: String,
    pub op: String,
    pub value: Value
}

#[derive(Debug, Clone)]
pub enum Expression {
    And(Box<Expression>, Box<Expression>),
    Or(Box<Expression>, Box<Expression>),
    Condition(Condition)
}

named!(operators<CompleteStr, CompleteStr>,
    do_parse!(
        tag: alt!(
            tag!("==") |
            tag!("!=") |
            tag!("=~") |
            tag!("=") |
            tag!("!~") |
            tag!("<") |
            tag!(">") |
            tag!("<=") |
            tag!(">=") |
            tag!("between") |
            tag!("#") |
            tag!("!#")
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
                digit >>
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
        low: number >>
        tag!(":") >>
        high: number >>
        (low, high)
    ).map(|result| {
        if let (Value::Int(low), Value::Int(high)) = result.1 {
            (result.0, Value::Range(low, high))
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
        let op = tuple.1.to_string();
        let value: Value = tuple.2;

        (
            result.0,
            Expression::Condition(
                Condition {
                    attribute, op, value
                }
            )
        )
    })
}

fn parens(input: CompleteStr) -> IResult<CompleteStr, Expression> {
    do_parse!(
        input,
        tag!("(")
        >> ret: alt!(expression | parens | condition)
        >> tag!(")")
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
        Ok(expr) => Ok(expr.1),
        Err(result) => {
            Err("Error parsing expression")
        }
    }
}
