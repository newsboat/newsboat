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
                    let low = std::cmp::min(*a, *b);
                    let high = std::cmp::max(*a,*b);
                    let i = attr.parse::<i32>().unwrap();
                    i >= low && i <= high
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

mod tests {
    use super::*;

    struct MockMatchable{}
    impl Matchable for MockMatchable {
        fn has_attribute(&self, attr: &str) -> bool {
            attr == "abcd" || attr == "AAAA" || attr == "tags"
        }
        fn get_attribute(&self, attr: &str) -> String {
            match attr {
                "abcd" => "xyz".to_string(),
                "AAAA" => "12345".to_string(),
                "tags" => "foo bar baz quux".to_string(),
                _ => "".to_string()
            }
        }
    }

    fn parse_and_match(expr: &str) -> Result<bool, String> {
        let m = Matcher::parse(expr)?.matches(MockMatchable{});
        Ok(m)
    }

    #[test]
    fn t_test_operators() {
        assert!(parse_and_match("abcd = \"xyz\"").unwrap());
        assert!(!parse_and_match("abcd = \"uiop\"").unwrap());

        assert!(parse_and_match("abcd != \"uiop\"").unwrap());
        assert!(!parse_and_match("abcd != \"xyz\"").unwrap());

        assert!(parse_and_match("AAAA =~ \".\"").unwrap());
        assert!(parse_and_match("AAAA =~ \"123\"").unwrap());
        assert!(parse_and_match("AAAA =~ \"234\"").unwrap());
        assert!(parse_and_match("AAAA =~ \"45\"").unwrap());
        assert!(parse_and_match("AAAA =~ \"^12345$\"").unwrap());
        assert!(parse_and_match("AAAA =~ \"^123456$\"").unwrap());
    }

    #[test]
    fn t_test_undefined_fields() {
        assert!(parse_and_match("BBBB =~ \"foo\"").is_err());
        assert!(parse_and_match("BBBB # \"foo\"").is_err());
        assert!(parse_and_match("BBBB < 0").is_err());
        assert!(parse_and_match("BBBB > 0").is_err());
        assert!(parse_and_match("BBBB between 1:23").is_err());
    }

    #[test]
    fn t_test_invalid_regex() {
        assert!(parse_and_match("AAAA =~ \"[[\"").is_err());
        assert!(parse_and_match("AAAA !~ \"[[\"").is_err());
    }

    #[test]
    fn t_test_regex_match() {
        assert!(parse_and_match("AAAA !~ \".\"").unwrap());
        assert!(parse_and_match("AAAA !~ \"123\"").unwrap());
        assert!(parse_and_match("AAAA !~ \"234\"").unwrap());
        assert!(parse_and_match("AAAA !~ \"45\"").unwrap());
        assert!(parse_and_match("AAAA !~ \"^12345$\"").unwrap());
    }

    #[test]
    fn t_test_contains() {
        assert!(parse_and_match("tags # \"foo\"").unwrap());
        assert!(parse_and_match("tags # \"baz\"").unwrap());
        assert!(parse_and_match("tags # \"quux\"").unwrap());
        assert!(!parse_and_match("tags # \"xyz\"").unwrap());
        assert!(!parse_and_match("tags # \"foo bar\"").unwrap());
        assert!(parse_and_match("tags # \"foo\" and tags # \"bar\"").unwrap());
        assert!(!parse_and_match("tags # \"foo\" and tags # \"xyz\"").unwrap());
        assert!(parse_and_match("tags # \"foo\" or tags # \"xyz\"").unwrap());

        assert!(parse_and_match("tags !# \"nein\"").unwrap());
        assert!(!parse_and_match("tags !# \"foo\"").unwrap());
    }

    #[test]
    fn t_test_comparisons() {
        assert!(parse_and_match("AAAA > 12344").unwrap());
        assert!(!parse_and_match("AAAA > 12345").unwrap());
        assert!(parse_and_match("AAAA >= 12345").unwrap());
        assert!(!parse_and_match("AAAA < 12345").unwrap());
        assert!(parse_and_match("AAAA <= 12345").unwrap());

        assert!(parse_and_match("AAAA between 0:12345").unwrap());
        assert!(parse_and_match("AAAA between 12345:12345").unwrap());
        assert!(!parse_and_match("AAAA between 23:12344").unwrap());
        assert!(!parse_and_match("AAAA between 0").unwrap());
        assert!(parse_and_match("AAAA between 12346:12344").unwrap());
    }
}
