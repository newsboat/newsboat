//! Checks if given filter expression is true for a given feed or article.

use crate::filterparser::{self, Expression, Expression::*, Operator, Value};
use crate::matchable::Matchable;
use crate::matchererror::MatcherError;
use regex_rs::{CompFlags, MatchFlags, Regex};

/// Checks if given filter expression is true for a given feed or article.
///
/// This is used for filters, query feeds, `ignore-article` commands, and even for hiding
/// already-read feeds and items.
pub struct Matcher {
    expr: Expression,

    /// Input string from which `expr` was created
    text: String,
}

impl Matcher {
    /// Prepare a `Matcher` that will check items against the filter expression provided in
    /// `input`.
    ///
    /// If `input` can't be parsed, returns a user-readable error.
    pub fn parse(input: &str) -> Result<Matcher, String> {
        let expr = filterparser::parse(input).map_err(|e| match e {
            filterparser::Error::TrailingCharacters(tail) => {
                format!("Parse error: trailing characters: {}", tail)
            }
            filterparser::Error::AtPos(pos) => format!("Parse error at position {}", pos),
        })?;

        Ok(Matcher {
            expr,
            text: input.to_string(),
        })
    }

    /// Check if given matchable `item` matches the filter.
    pub fn matches(&self, item: &impl Matchable) -> Result<bool, MatcherError> {
        evaluate_expression(&self.expr, item)
    }

    /// The filter expression from which this `Matcher` was constructed.
    pub fn get_expression(&self) -> &str {
        &self.text
    }
}

impl Operator {
    fn apply(&self, attr: &str, value: &Value) -> Result<bool, MatcherError> {
        match self {
            Operator::Equals => match value {
                Value::Str(ref s) => Ok(attr == s),
                Value::Int(i) => match attr.parse::<i32>() {
                    Ok(attr) => Ok(attr == *i),
                    _ => Ok(false),
                },
                _ => Ok(false),
            },
            Operator::NotEquals => Operator::Equals
                .apply(attr, value)
                .and_then(|result| Ok(!result)),
            Operator::RegexMatches => match value {
                Value::Str(ref pattern) => {
                    match Regex::new(
                        pattern,
                        CompFlags::EXTENDED | CompFlags::IGNORE_CASE | CompFlags::NO_SUB,
                    ) {
                        Ok(regex) => {
                            let mut result = false;
                            let max_matches = 1;
                            if let Ok(matches) =
                                regex.matches(attr, max_matches, MatchFlags::empty())
                            {
                                // Ok with non-empty Vec inside means a match was found
                                result = !matches.is_empty();
                            }
                            Ok(result)
                        }

                        Err(errmsg) => Err(MatcherError::InvalidRegex {
                            regex: pattern.clone(),
                            errmsg,
                        }),
                    }
                }
                _ => Ok(false),
            },
            Operator::NotRegexMatches => Operator::RegexMatches
                .apply(attr, value)
                .and_then(|result| Ok(!result)),
            Operator::LessThan => match value {
                Value::Int(i) => Ok(attr.parse::<i32>().unwrap() < *i),
                _ => Ok(false),
            },
            Operator::GreaterThan => match value {
                Value::Int(i) => Ok(attr.parse::<i32>().unwrap() > *i),
                _ => Ok(true),
            },
            Operator::LessThanOrEquals => match value {
                Value::Int(i) => Ok(attr.parse::<i32>().unwrap() <= *i),
                _ => Ok(false),
            },
            Operator::GreaterThanOrEquals => match value {
                Value::Int(i) => Ok(attr.parse::<i32>().unwrap() >= *i),
                _ => Ok(true),
            },
            Operator::Between => match value {
                Value::Range(a, b) => {
                    let low = std::cmp::min(*a, *b);
                    let high = std::cmp::max(*a, *b);
                    let i = attr.parse::<i32>().unwrap();
                    Ok(i >= low && i <= high)
                }
                _ => Ok(false),
            },
            Operator::Contains => match value {
                Value::Int(i) => self.apply(attr, &Value::Str(format!("{}", i))),
                Value::Str(ref s) => {
                    for token in attr.split(' ') {
                        if token == s {
                            return Ok(true);
                        }
                    }
                    Ok(false)
                }
                _ => Ok(false),
            },
            Operator::NotContains => Operator::Contains
                .apply(attr, value)
                .and_then(|result| Ok(!result)),
        }
    }
}

fn evaluate_expression(expr: &Expression, item: &impl Matchable) -> Result<bool, MatcherError> {
    match expr {
        Comparison {
            attribute,
            op,
            value,
        } => match item.attribute_value(&attribute) {
            None => Err(MatcherError::AttributeUnavailable {
                attr: attribute.clone(),
            }),

            Some(ref attr) => op.apply(attr, &value),
        },
        And(left, right) => evaluate_expression(left, item).and_then(|result| {
            if result {
                evaluate_expression(right, item)
            } else {
                Ok(false)
            }
        }),
        Or(left, right) => evaluate_expression(left, item).and_then(|result| {
            if result {
                Ok(true)
            } else {
                evaluate_expression(right, item)
            }
        }),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::collections::BTreeMap;

    struct MockMatchable {
        values: BTreeMap<String, String>,
    }

    impl MockMatchable {
        pub fn new(values: &[(&str, &str)]) -> MockMatchable {
            MockMatchable {
                values: values
                    .into_iter()
                    .map(|(a, b)| (String::from(*a), String::from(*b)))
                    .collect::<BTreeMap<_, _>>(),
            }
        }
    }

    impl Matchable for MockMatchable {
        fn attribute_value(&self, attr: &str) -> Option<String> {
            self.values.get(attr).cloned()
        }
    }

    #[test]
    fn t_test_equality_works_with_strings() {
        let mock = MockMatchable::new(&[("abcd", "xyz")]);
        assert!(Matcher::parse("abcd = \"xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("abcd = \"uiop\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_equality_works_with_numbers() {
        let mock = MockMatchable::new(&[("answer", "42"), ("agent", "007")]);
        assert!(Matcher::parse("answer = 42")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("answer = 0042")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("answer = 13")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("agent = 7").unwrap().matches(&mock).unwrap());
        assert!(Matcher::parse("agent = 007")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_equality_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("answer", "42")]);
        assert!(!Matcher::parse("answer = 0:100")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("answer = 100:200")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("answer = 42:200")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("answer = 0:42")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_nonequality_works_with_strings() {
        let mock = MockMatchable::new(&[("abcd", "xyz")]);
        assert!(Matcher::parse("abcd != \"uiop\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("abcd != \"xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_nonequality_works_with_numbers() {
        let mock = MockMatchable::new(&[("answer", "42"), ("agent", "007")]);
        assert!(Matcher::parse("answer != 13")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("answer != 42")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("agent != 7")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("agent != 007")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_nonequality_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("answer", "42")]);
        assert!(Matcher::parse("answer != 0:100")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("answer != 100:200")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("answer != 42:200")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("answer != 0:42")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_regex_match_works_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA =~ \".\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA =~ \"123\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA =~ \"234\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA =~ \"45\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA =~ \"^12345$\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA =~ \"^123456$\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_regex_match_doesnt_work_with_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(!Matcher::parse("AAAA =~ 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA =~ 1").unwrap().matches(&mock).unwrap());
        assert!(!Matcher::parse("AAAA =~ 45")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA =~ 9").unwrap().matches(&mock).unwrap());
    }

    #[test]
    fn t_test_regex_match_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345"), ("range", "0:123")]);

        assert!(!Matcher::parse("AAAA =~ 0:123456")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA =~ 12345:99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA =~ 0:12345")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("range =~ 0:123")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("range =~ 0:12")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("range =~ 0:1234")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_error_on_undefined_fields() {
        let mock = MockMatchable::new(&[]);

        let check = |expression| {
            match Matcher::parse(expression).unwrap().matches(&mock) {
                Err(MatcherError::AttributeUnavailable { .. }) => { /* that's the expected result */
                }
                result => panic!(format!("unexpected result: {:?}", result)),
            }
        };

        check("BBBB = 1");
        check("BBBB =~ \"foo\"");
        check("BBBB # \"foo\"");
        check("BBBB < 0");
        check("BBBB > 0");
        check("BBBB between 1:23");
    }

    #[test]
    fn t_error_on_invalid_regex() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        match Matcher::parse("AAAA =~ \"[[\"").unwrap().matches(&mock) {
            Err(MatcherError::InvalidRegex { .. }) => { /* that's the expected result */ }
            result => panic!(format!("unexpected result: {:?}", result)),
        }

        match Matcher::parse("AAAA !~ \"[[\"").unwrap().matches(&mock) {
            Err(MatcherError::InvalidRegex { .. }) => { /* that's the expected result */ }
            result => panic!(format!("unexpected result: {:?}", result)),
        }
    }

    #[test]
    fn t_test_not_regex_match_works_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(!Matcher::parse("AAAA !~ \".\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA !~ \"123\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA !~ \"234\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA !~ \"45\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA !~ \"^12345$\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("AAAA !~ \"567\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA !~ \"number\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_not_regex_match_doesnt_work_with_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA !~ 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA !~ 1").unwrap().matches(&mock).unwrap());
        assert!(Matcher::parse("AAAA !~ 45")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA !~ 9").unwrap().matches(&mock).unwrap());
    }

    #[test]
    fn t_test_not_regex_match_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA !~ 0:123456")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA !~ 12345:99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA !~ 0:12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_contains_works_with_strings() {
        let mock = MockMatchable::new(&[("tags", "foo bar baz quux")]);

        assert!(Matcher::parse("tags # \"foo\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("tags # \"baz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("tags # \"quux\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("tags # \"uu\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("tags # \"xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("tags # \"foo bar\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("tags # \"foo\" and tags # \"bar\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("tags # \"foo\" and tags # \"xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("tags # \"foo\" or tags # \"xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_contains_works_with_numbers() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(Matcher::parse("fibonacci # 1")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci # 3")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("fibonacci # 4")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_contains_converts_numbers_to_strings_to_look_them_up() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(Matcher::parse("fibonacci # \"1\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci # \"3\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("fibonacci # \"4\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_contains_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(!Matcher::parse("fibonacci # 1:5")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("fibonacci # 3:100")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_contains_works_with_single_value_lists() {
        let mock = MockMatchable::new(&[("values", "one"), ("number", "1")]);

        assert!(Matcher::parse("values # \"one\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("number # 1")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_not_contains_works_with_strings() {
        let mock = MockMatchable::new(&[("tags", "foo bar baz quux")]);

        assert!(Matcher::parse("tags !# \"nein\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("tags !# \"foo\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_not_contains_works_with_numbers() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(!Matcher::parse("fibonacci !# 1")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci !# 9")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci !# 4")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_not_contains_converts_numbers_to_strings_to_look_them_up() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(!Matcher::parse("fibonacci !# \"1\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci !# \"9\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci !# \"4\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_not_contains_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(Matcher::parse("fibonacci !# 1:5")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("fibonacci !# 7:35")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_not_contains_works_even_on_single_value_lists() {
        let mock = MockMatchable::new(&[("values", "one"), ("number", "1")]);

        assert!(!Matcher::parse("values !# \"one\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("values !# \"two\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("number !# 1")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("number !# 2")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_comparisons_dont_work_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA > \"12344\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("AAAA > \"12345\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("AAAA > \"123456\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("AAAA < \"12345\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("AAAA < \"12346\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("AAAA < \"123456\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("AAAA >= \"12344\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("AAAA >= \"12345\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(Matcher::parse("AAAA >= \"12346\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("AAAA <= \"12344\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("AAAA <= \"12345\"")
            .unwrap()
            .matches(&mock)
            .unwrap());

        assert!(!Matcher::parse("AAAA <= \"12346\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_comparisons_work_with_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA > 12344")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA > 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA >= 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA < 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA <= 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_comparisons_dont_work_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA > 0:99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA < 0:99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA >= 0:99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA <= 0:99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_operator_between_doesnt_work_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(!Matcher::parse("AAAA between \"123\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA between \"12399\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_operator_between_doesnt_work_with_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(!Matcher::parse("AAAA between 1")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA between 12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA between 99999")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_test_operator_between_works_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(Matcher::parse("AAAA between 0:12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA between 12345:12345")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(!Matcher::parse("AAAA between 23:12344")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("AAAA between 12346:12344")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_get_expression_method_returns_parsed_expression_as_string() {
        let expression = "AAAA between 1:30000";
        let matcher = Matcher::parse(expression).unwrap();
        assert_eq!(matcher.get_expression(), expression);
    }

    #[test]
    fn t_regexes_matched_case_insensitively() {
        // Inspired by https://github.com/newsboat/newsboat/issues/642

        let mock = MockMatchable::new(&[("abcd", "xyz")]);

        assert!(Matcher::parse("abcd =~ \"xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"xYz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"xYZ\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"Xyz\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"yZ\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"^xYZ\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"xYz$\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"^Xyz$\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"^xY\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse("abcd =~ \"yZ$\"")
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    // The following tests check if our regex matching uses POSIX extended regular expression
    // syntax. That syntax is documented in The Open Group Base Specifications Issue 7,
    // IEEE Std 1003.1-2008 Section 9, "Regular Expressions":
    // https://pubs.opengroup.org/onlinepubs/9699919799.2008edition/basedefs/V1_chap09.html
    //
    // Since POSIX extended regular expressions are pretty basic, it's hard to
    // find stuff that they support but other engines don't. So in order to
    // ensure that we're using EREs, these tests try stuff that's *not*
    // supported by EREs.
    //
    // Ideas gleaned from https://www.regular-expressions.info/refcharacters.html

    #[test]
    fn t_regex_matching_doesnt_support_escape_sequence() {
        // Supported by Perl, PCRE, PHP and others, but not POSIX ERE

        let mock = MockMatchable::new(&[("attr", "*]+")]);

        assert!(!Matcher::parse(r#"(attr =~ "\Q*]+\E")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse(r#"(attr !~ "\Q*]+\E")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_regex_matching_doesnt_support_hexadecimal_escape() {
        let mock = MockMatchable::new(&[("attr", "value")]);

        assert!(!Matcher::parse(r#"(attr =~ "^va\x6Cue")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse(r#"attr !~ "^va\x6Cue""#)
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_regex_matching_doesnt_support_backslash_a_as_alert_control_character() {
        let mock = MockMatchable::new(&[("attr", "\x07")]);

        assert!(!Matcher::parse(r#"(attr =~ "\a")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse(r#"(attr !~ "\a")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
    }

    #[test]
    fn t_regex_matching_doesnt_support_backslash_b_as_backspace_control_character() {
        let mock = MockMatchable::new(&[("attr", "\x08")]);

        assert!(!Matcher::parse(r#"(attr =~ "\b")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
        assert!(Matcher::parse(r#"(attr !~ "\b")"#)
            .unwrap()
            .matches(&mock)
            .unwrap());
    }
}
