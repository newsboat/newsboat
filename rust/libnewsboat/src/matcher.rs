use filterparser;
use filterparser::Operation;
use filterparser::Condition;
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
            Operation::Equal => match value {
                Value::Str(ref s) => attr == s,
                _ => false
            },
            Operation::NotEqual => !Operation::Equal.apply(attr, value),
            Operation::RegEqual => match value {
                Value::Str(ref s) => false, // TODO: Implement,
                _ => false
            },
            Operation::NotRegEqual => !Operation::RegEqual.apply(attr, value),
            Operation::LessThan => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() < *i,
                _ => false
            },
            Operation::GreaterThan => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() > *i,
                _ => false
            },
            Operation::LessThanOrEqual => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() <= *i,
                _ => false
            },
            Operation::GreaterThanOrEqual => match value {
                Value::Int(i) => attr.parse::<i32>().unwrap() >= *i,
                _ => false
            },
            Operation::Between => match value {
                Value::Range(a,b) => {
                    let i = attr.parse::<i32>().unwrap();
                    i > *a && i < *b
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
        Condition(cond) => check_condition(cond, item),
        And(left, right) => evaluate_expression(left, item) && evaluate_expression(right, item),
        Or(left, right) => evaluate_expression(left, item) || evaluate_expression(right, item),
    }
}

fn check_condition(cond: &Condition, item: &impl Matchable) -> bool{
    if !item.has_attribute(&cond.attribute) {
        return false;
    }

    let ref attr = item.get_attribute(&cond.attribute);

    cond.op.apply(attr, &cond.value)
}
