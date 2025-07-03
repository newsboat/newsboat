//! Checks if given filter expression is true for a given feed or article.

use crate::filterparser::{self, Expression, Expression::*, Operator, Value};
use crate::matchable::Matchable;
use crate::matchererror::MatcherError;

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
    /// If `input` can't be parsed, returns an internalized error message.
    pub fn parse(input: &str) -> Result<Matcher, String> {
        let expr = filterparser::parse(input)?;
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
            Operator::Equals => Ok(attr == value.literal()),
            Operator::NotEquals => Operator::Equals.apply(attr, value).map(|result| !result),
            Operator::RegexMatches => match value.as_regex() {
                Ok(regex) => {
                    let mut result = false;
                    let max_matches = 1;
                    if let Ok(matches) =
                        regex.matches(attr, max_matches, regex_rs::MatchFlags::empty())
                    {
                        // Ok with non-empty Vec inside means a match was found
                        result = !matches.is_empty();
                    }
                    Ok(result)
                }

                Err(errmsg) => Err(MatcherError::InvalidRegex {
                    regex: value.literal().to_string(),
                    errmsg: errmsg.to_string(),
                }),
            },
            Operator::NotRegexMatches => Operator::RegexMatches
                .apply(attr, value)
                .map(|result| !result),
            Operator::LessThan => Ok(string_to_num(attr) < string_to_num(value.literal())),
            Operator::GreaterThan => Ok(string_to_num(attr) > string_to_num(value.literal())),
            Operator::LessThanOrEquals => Ok(string_to_num(attr) <= string_to_num(value.literal())),
            Operator::GreaterThanOrEquals => {
                Ok(string_to_num(attr) >= string_to_num(value.literal()))
            }
            Operator::Between => {
                let fields = value.literal().split(':').collect::<Vec<_>>();
                if fields.len() != 2 {
                    return Ok(false);
                }

                let a = string_to_num(fields[0]);
                let b = string_to_num(fields[1]);

                let low = std::cmp::min(a, b);
                let high = std::cmp::max(a, b);
                let i = string_to_num(attr);
                Ok(i >= low && i <= high)
            }
            Operator::Contains => {
                for token in attr.split(' ') {
                    if token == value.literal() {
                        return Ok(true);
                    }
                }
                Ok(false)
            }
            Operator::NotContains => Operator::Contains.apply(attr, value).map(|result| !result),
        }
    }
}

/// Convert numerical prefix of the string to i32.
///
/// Return 0 if there is no numeric prefix. On underflow, return `i32::MIN`. On overflow,
/// return `i32::MAX`.
fn string_to_num(input: &str) -> i32 {
    let search_start = usize::from(input.starts_with('-'));

    let numerics_end = input[search_start..]
        .find(|c: char| !c.is_numeric())
        .unwrap_or(input.len())
        + search_start; // Adding the starting offset to get an index inside the original input

    if numerics_end - search_start == 0 {
        // No numeric prefix
        return 0;
    }

    if let Ok(number) = input[..numerics_end].parse::<i32>() {
        number
    } else if search_start == 1 {
        // Number starts with minus and couldn't be parsed => underflow
        i32::MIN
    } else {
        // Number doesn't start with minus and couldn't be parsed => overflow
        i32::MAX
    }
}

fn evaluate_expression(expr: &Expression, item: &impl Matchable) -> Result<bool, MatcherError> {
    match expr {
        Comparison {
            attribute,
            op,
            value,
        } => match item.attribute_value(attribute) {
            None => Err(MatcherError::AttributeUnavailable {
                attr: attribute.clone(),
            }),

            Some(ref attr) => op.apply(attr, value),
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
                    .iter()
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
        assert!(
            Matcher::parse("abcd = \"xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("abcd = \"uiop\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_equality_works_with_numbers() {
        let mock = MockMatchable::new(&[("answer", "42"), ("agent", "007")]);
        assert!(
            Matcher::parse("answer = 42")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("answer = 0042")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("answer = 13")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(!Matcher::parse("agent = 7").unwrap().matches(&mock).unwrap());
        assert!(
            Matcher::parse("agent = 007")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_equality_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("answer", "42")]);
        assert!(
            !Matcher::parse("answer = 0:100")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("answer = 100:200")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("answer = 42:200")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("answer = 0:42")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_nonequality_works_with_strings() {
        let mock = MockMatchable::new(&[("abcd", "xyz")]);
        assert!(
            Matcher::parse("abcd != \"uiop\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("abcd != \"xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_nonequality_works_with_numbers() {
        let mock = MockMatchable::new(&[("answer", "42"), ("agent", "007")]);
        assert!(
            Matcher::parse("answer != 13")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("answer != 42")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("agent != 7")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("agent != 007")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_nonequality_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("answer", "42")]);
        assert!(
            Matcher::parse("answer != 0:100")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("answer != 100:200")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("answer != 42:200")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("answer != 0:42")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_regex_match_works_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA =~ \".\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA =~ \"123\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA =~ \"234\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA =~ \"45\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA =~ \"^12345$\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA =~ \"^123456$\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_regex_match_converts_numbers_to_strings_and_uses_them_as_regexes() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA =~ 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(Matcher::parse("AAAA =~ 1").unwrap().matches(&mock).unwrap());
        assert!(
            Matcher::parse("AAAA =~ 45")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(!Matcher::parse("AAAA =~ 9").unwrap().matches(&mock).unwrap());
    }

    #[test]
    fn t_test_regex_match_treats_ranges_as_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345"), ("range", "0:123")]);

        assert!(
            !Matcher::parse("AAAA =~ 0:123456")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA =~ 12345:99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA =~ 0:12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("range =~ 0:123")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("range =~ 0:12")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("range =~ 0:1234")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_error_on_undefined_fields() {
        let mock = MockMatchable::new(&[]);

        let check = |expression| {
            match Matcher::parse(expression).unwrap().matches(&mock) {
                Err(MatcherError::AttributeUnavailable { .. }) => { /* that's the expected result */
                }
                result => panic!("unexpected result: {:?}", result),
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
            result => panic!("unexpected result: {:?}", result),
        }

        match Matcher::parse("AAAA !~ \"[[\"").unwrap().matches(&mock) {
            Err(MatcherError::InvalidRegex { .. }) => { /* that's the expected result */ }
            result => panic!("unexpected result: {:?}", result),
        }
    }

    #[test]
    fn t_test_not_regex_match_works_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            !Matcher::parse("AAAA !~ \".\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA !~ \"123\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA !~ \"234\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA !~ \"45\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA !~ \"^12345$\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA !~ \"567\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA !~ \"number\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_not_regex_match_converts_numbers_into_strings_and_uses_them_as_regexes() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            !Matcher::parse("AAAA !~ 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(!Matcher::parse("AAAA !~ 1").unwrap().matches(&mock).unwrap());
        assert!(
            !Matcher::parse("AAAA !~ 45")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(Matcher::parse("AAAA !~ 9").unwrap().matches(&mock).unwrap());
    }

    #[test]
    fn t_test_not_regex_match_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA !~ 0:123456")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA !~ 12345:99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA !~ 0:12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_contains_works_with_strings() {
        let mock = MockMatchable::new(&[("tags", "foo bar baz quux")]);

        assert!(
            Matcher::parse("tags # \"foo\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("tags # \"baz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("tags # \"quux\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("tags # \"uu\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("tags # \"xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("tags # \"foo bar\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("tags # \"foo\" and tags # \"bar\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("tags # \"foo\" and tags # \"xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("tags # \"foo\" or tags # \"xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_contains_works_with_numbers() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(
            Matcher::parse("fibonacci # 1")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci # 3")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("fibonacci # 4")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_contains_converts_numbers_to_strings_to_look_them_up() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(
            Matcher::parse("fibonacci # \"1\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci # \"3\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("fibonacci # \"4\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_contains_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(
            !Matcher::parse("fibonacci # 1:5")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("fibonacci # 3:100")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_contains_works_with_single_value_lists() {
        let mock = MockMatchable::new(&[("values", "one"), ("number", "1")]);

        assert!(
            Matcher::parse("values # \"one\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("number # 1")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_not_contains_works_with_strings() {
        let mock = MockMatchable::new(&[("tags", "foo bar baz quux")]);

        assert!(
            Matcher::parse("tags !# \"nein\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("tags !# \"foo\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_not_contains_works_with_numbers() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(
            !Matcher::parse("fibonacci !# 1")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci !# 9")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci !# 4")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_not_contains_converts_numbers_to_strings_to_look_them_up() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(
            !Matcher::parse("fibonacci !# \"1\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci !# \"9\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci !# \"4\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_not_contains_doesnt_work_with_ranges() {
        let mock = MockMatchable::new(&[("fibonacci", "1 1 2 3 5 8 13 21 34")]);

        assert!(
            Matcher::parse("fibonacci !# 1:5")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("fibonacci !# 7:35")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_not_contains_works_even_on_single_value_lists() {
        let mock = MockMatchable::new(&[("values", "one"), ("number", "1")]);

        assert!(
            !Matcher::parse("values !# \"one\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("values !# \"two\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("number !# 1")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("number !# 2")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_comparisons_convert_string_arguments_to_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA > \"12344\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            !Matcher::parse("AAAA > \"12345\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            !Matcher::parse("AAAA > \"123456\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            !Matcher::parse("AAAA < \"12345\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA < \"12346\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA < \"123456\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA >= \"12344\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA >= \"12345\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            !Matcher::parse("AAAA >= \"12346\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            !Matcher::parse("AAAA <= \"12344\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA <= \"12345\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );

        assert!(
            Matcher::parse("AAAA <= \"12346\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_comparisons_use_numeric_prefix_of_the_string() {
        let mock = MockMatchable::new(&[("AAAA", "12345xx")]);

        assert!(
            Matcher::parse("AAAA >= \"12345\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA > \"1234a\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA < \"12345a\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA < \"1234a\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA < \"9999b\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_comparisons_use_zero_if_string_cant_be_converted_to_number() {
        let mock = MockMatchable::new(&[("zero", "0"), ("same_zero", "yeah")]);

        assert!(
            !Matcher::parse("zero < \"unknown\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("zero > \"unknown\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("zero <= \"unknown\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("zero >= \"unknown\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("same_zero < \"0\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("same_zero > \"0\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("same_zero <= \"0\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("same_zero >= \"0\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_comparisons_work_with_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA > 12344")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA > 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA >= 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA < 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA <= 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_comparisons_dont_work_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA > 0:99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA < 0:99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA >= 0:99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA <= 0:99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_operator_between_doesnt_work_with_strings() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            !Matcher::parse("AAAA between \"123\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA between \"12399\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_operator_between_doesnt_work_with_numbers() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            !Matcher::parse("AAAA between 1")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA between 12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA between 99999")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_operator_between_works_with_ranges() {
        let mock = MockMatchable::new(&[("AAAA", "12345")]);

        assert!(
            Matcher::parse("AAAA between 0:12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA between 12345:12345")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("AAAA between 23:12344")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("AAAA between 12346:12344")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
    }

    #[test]
    fn t_test_operator_between_converts_numeric_prefix_of_the_attribute() {
        let mock = MockMatchable::new(&[("value", "123four"), ("practically_zero", "sure")]);

        assert!(
            Matcher::parse("value between 122:124")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("value between 124:130")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("practically_zero between 0:1")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            !Matcher::parse("practically_zero between 1:100")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
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

        assert!(
            Matcher::parse("abcd =~ \"xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"xYz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"xYZ\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"Xyz\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"yZ\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"^xYZ\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"xYz$\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"^Xyz$\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"^xY\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
        assert!(
            Matcher::parse("abcd =~ \"yZ$\"")
                .unwrap()
                .matches(&mock)
                .unwrap()
        );
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
    // FreeBSD 13 returns an error from regexec() instead of failing the match like other OSes do.
    // That's why we do the `match` instead of simply chaining the results with `unwrap()`.
    //
    // Ideas gleaned from https://www.regular-expressions.info/refcharacters.html

    #[test]
    fn t_regex_matching_doesnt_support_escape_sequence() {
        // Supported by Perl, PCRE, PHP and others, but not POSIX ERE

        let mock = MockMatchable::new(&[("attr", "*]+")]);

        match Matcher::parse(r#"(attr =~ "\Q*]+\E")"#)
            .unwrap()
            .matches(&mock)
        {
            Ok(result) => assert!(!result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }

        match Matcher::parse(r#"(attr !~ "\Q*]+\E")"#)
            .unwrap()
            .matches(&mock)
        {
            Ok(result) => assert!(result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }
    }

    #[test]
    fn t_regex_matching_doesnt_support_hexadecimal_escape() {
        let mock = MockMatchable::new(&[("attr", "value")]);

        match Matcher::parse(r#"(attr =~ "^va\x6Cue")"#)
            .unwrap()
            .matches(&mock)
        {
            Ok(result) => assert!(!result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }

        match Matcher::parse(r#"attr !~ "^va\x6Cue""#)
            .unwrap()
            .matches(&mock)
        {
            Ok(result) => assert!(result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }
    }

    #[test]
    fn t_regex_matching_doesnt_support_backslash_a_as_alert_control_character() {
        let mock = MockMatchable::new(&[("attr", "\x07")]);

        match Matcher::parse(r#"(attr =~ "\a")"#).unwrap().matches(&mock) {
            Ok(result) => assert!(!result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }

        match Matcher::parse(r#"(attr !~ "\a")"#).unwrap().matches(&mock) {
            Ok(result) => assert!(result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }
    }

    #[test]
    fn t_regex_matching_doesnt_support_backslash_b_as_backspace_control_character() {
        let mock = MockMatchable::new(&[("attr", "\x08")]);

        match Matcher::parse(r#"(attr =~ "\b")"#).unwrap().matches(&mock) {
            Ok(result) => assert!(!result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }

        match Matcher::parse(r#"(attr !~ "\b")"#).unwrap().matches(&mock) {
            Ok(result) => assert!(result),
            Err(e) => assert!(matches!(e, MatcherError::InvalidRegex { .. })),
        }
    }

    #[test]
    fn t_string_to_num_convers_numeric_prefix_to_i32() {
        assert_eq!(string_to_num("7654"), 7654);
        assert_eq!(string_to_num("123foo"), 123);
        assert_eq!(string_to_num("-999999bar"), -999999);

        assert_eq!(string_to_num("-2147483648min"), -2147483648);
        assert_eq!(string_to_num("2147483647 is ok"), 2147483647);

        // On under-/over-flow, returns min/max representable value
        assert_eq!(
            string_to_num("-2147483649 is too small for i32"),
            -2147483648
        );
        assert_eq!(string_to_num("2147483648 is too large for i32"), 2147483647);
    }

    #[test]
    fn t_string_to_num_returns_0_if_there_is_no_numeric_prefix() {
        assert_eq!(string_to_num("hello"), 0);
        assert_eq!(string_to_num(""), 0);
    }
}
