use filterparser;
use filterparser::Operation;
use filterparser::Comparison;
use filterparser::Value;
use filterparser::Expression;
use filterparser::Expression::*;

use logger::{self, Level};

pub trait Matchable {
    fn has_attribute(&self, attr: &str) -> bool;
    fn get_attribute(&self, attr: &str) -> String;
}

pub struct Matcher {
    expr: Expression,
}

impl Matcher {
    pub fn parse(expr: &str) -> Result<Matcher, &str> {
        let expr = filterparser::parse(expr)?;
        log!(Level::Debug, "expr: {:#?}", expr);
        Ok(Matcher {
            expr
        })
    }
    pub fn matches(&self, item: impl Matchable) -> bool {
        evaluate_expression(&self.expr, &item)
    }
}

impl Operation {
    fn apply(&self, attr: &str, value: &Value) -> bool {
        match self {
            Operation::Equals => match value {
                Value::Str(ref s) => attr == s,
                _ => false
            },
            Operation::NotEquals => !Operation::Equals.apply(attr, value),
            Operation::RegEquals => match value {
                Value::Str(ref s) => false, // TODO: Implement,
                _ => false
            },
            Operation::NotRegEquals => !Operation::RegEquals.apply(attr, value),
            Operation::LessThan => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() < *i,
                _ => false
            },
            Operation::GreaterThan => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() > *i,
                _ => false
            },
            Operation::LessThanOrEquals => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() <= *i,
                _ => false
            },
            Operation::GreaterThanOrEquals => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() >= *i,
                _ => false
            },
            Operation::Between => match value {
                Value::Range(a,b) => {
                    let i = attr.parse::<i32>().unwrap();
                    i >= *a && i <= *b
                }
                _ => false
            },
            Operation::Contains => match value {
                Value::Str(ref s) => {
                    for token in attr.split(" ") {
                        if token == s {
                            return true;
                        }
                    }
                    return false;
                }
                _ => false
            },
            Operation::NotContains => match value {
                Value::Str(ref s) => !Operation::Contains.apply(attr, value),
                _ => false
            },

        }
    }
}

fn evaluate_expression(expr: &Expression, item: &impl Matchable) -> bool {
    match expr {
        Comparison(cond) => check_comparison(cond, item),
        And(left, right) => evaluate_expression(left, item) && evaluate_expression(right, item),
        Or(left, right) => evaluate_expression(left, item) || evaluate_expression(right, item),
    }
}

fn check_comparison(cond: &Comparison, item: &impl Matchable) -> bool{
    if !item.has_attribute(&cond.attribute) {
        return false;
    }

    let ref attr = item.get_attribute(&cond.attribute);

    cond.op.apply(attr, &cond.value)
}
