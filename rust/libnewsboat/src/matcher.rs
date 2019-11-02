use filterparser;
use filterparser::Condition;
use filterparser::Value;
use filterparser::Expression;
use filterparser::Expression::*;

pub trait Matchable {
    fn has_attribute(&self, attr: &str) -> bool;
    fn get_attribute(&self, attr: &str) -> &str;
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

    match cond.value {
        Value::Str(ref s) => attr == s,
        _ => panic!("Invalid type!")
    }
}
