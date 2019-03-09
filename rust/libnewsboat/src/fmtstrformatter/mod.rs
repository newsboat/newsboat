//! Produces strings of values in a specified format, strftime(3)-like.

mod limited_string;
mod parser;

use self::limited_string::LimitedString;
use self::parser::{parse, Padding, Specifier};
use std::cmp::min;
use std::collections::BTreeMap;
use utils;

/// Produces strings of values in a specified format, strftime(3)-like.
///
/// Newsboat lets users customize its appearance using "format strings". These strings consist of
/// "format specifiers", each of which stand for some fact about the item.
///
/// For example, format strings dictate how each line of an article list look like. `%t` might mean
/// "article title", and `%a` might mean "article's author". Thus, a format string of `"%t (%a)"`
/// would be turned into "How I Spent My Summer (John Doe)" for one article and into "This Week in
/// Rust (Rust Project)" for another.
///
/// Since Newsboat contains a number of different format strings with different specifiers, we need
/// an object to encapsulate the common part: the act of formatting. That way, the above example
/// with titles and authors can be implemented very easily:
/// ```
/// use libnewsboat::fmtstrformatter::*;
///
/// let format = "%t (%a)";
/// let mut fmt = FmtStrFormatter::new();
///
/// // First example
/// {
///     fmt.register_fmt('a', "John Doe".to_string());
///     fmt.register_fmt('t', "How I Spent My Summer".to_string());
///
///     assert_eq!(fmt.do_format(format, 0), "How I Spent My Summer (John Doe)");
/// }
///
/// // Second example
/// {
///     fmt.register_fmt('t', "This Week in Rust".to_string());
///     fmt.register_fmt('a', "Rust Project".to_string());
///
///     assert_eq!(fmt.do_format(format, 0), "This Week in Rust (Rust Project)");
/// }
/// ```
///
/// We also have specifiers for different kinds of padding, and a conditional specifier that's
/// replaced by different values depending on the third one. See the Newsboat documentation for
/// details.
///
/// For the rest of this doc, we'll refer to letters like `a` and `t` above as "keys", and `John
/// Doe` etc. would be "values". The term "format specifiers" will be reserved to things like `%a`,
/// and "format strings" would mean a collection of format specifiers, with optional text in
/// between.
pub struct FmtStrFormatter {
    /// Stores keys and their values.
    fmts: BTreeMap<char, String>,
}

impl FmtStrFormatter {
    /// Construct new `FmtStrFormatter` that contains no keys and no values.
    pub fn new() -> FmtStrFormatter {
        FmtStrFormatter {
            fmts: BTreeMap::new(),
        }
    }

    /// Adds a key-value pair to the formatter.
    pub fn register_fmt(&mut self, key: char, value: String) {
        self.fmts.insert(key, value);
    }

    /// Takes a format string and replaces format specifiers with their values.
    pub fn do_format(&self, format: &str, width: u32) -> String {
        let ast = parse(format);
        self.formatting_helper(&ast, width)
    }

    fn format_spacing(&self, c: char, rest: &[Specifier], width: u32, result: &mut LimitedString) {
        let rest = self.formatting_helper(rest, 0);
        if width == 0 {
            result.push(c);
        } else {
            let padding_width = {
                let content_width = utils::graphemes_count(&rest) + result.length();
                if content_width > width as usize {
                    0
                } else {
                    width as usize - content_width
                }
            };

            let padding = format!("{}", c).repeat(padding_width);
            result.push_str(&padding);
        };
        result.push_str(&rest);
    }

    fn format_format(&self, c: char, padding: &Padding, _width: u32, result: &mut LimitedString) {
        let empty_string = String::new();
        let value = self.fmts.get(&c).unwrap_or_else(|| &empty_string);
        match padding {
            &Padding::None => result.push_str(value),

            &Padding::Left(total_width) => {
                let padding_width = total_width - min(total_width, utils::graphemes_count(value));
                let stripping_width = total_width - padding_width;
                let padding = String::from(" ").repeat(padding_width);
                result.push_str(&padding);
                result.push_str(&utils::take_graphemes(value, stripping_width));
            }

            &Padding::Right(total_width) => {
                let padding_width = total_width - min(total_width, utils::graphemes_count(value));
                let stripping_width = total_width - padding_width;
                let padding = String::from(" ").repeat(padding_width);
                result.push_str(&utils::take_graphemes(value, stripping_width));
                result.push_str(&padding);
            }
        }
    }

    fn format_conditional(
        &self,
        cond: char,
        then: &[Specifier],
        els: &Option<Vec<Specifier>>,
        width: u32,
        result: &mut LimitedString,
    ) {
        match self.fmts.get(&cond) {
            Some(value) if !value.is_empty() => {
                result.push_str(&self.formatting_helper(then, width))
            }
            _ => {
                if let &Some(ref els) = els {
                    result.push_str(&self.formatting_helper(&els, width))
                }
            }
        }
    }

    fn formatting_helper(&self, format_ast: &[Specifier], width: u32) -> String {
        let mut result = LimitedString::new(if width == 0 {
            None
        } else {
            Some(width as usize)
        });

        for (i, specifier) in format_ast.iter().enumerate() {
            match specifier {
                &Specifier::Spacing(c) => {
                    let rest = &format_ast[i + 1..];
                    self.format_spacing(c, rest, width, &mut result);
                    // format_spacing will also format the rest of the string, so quit the loop
                    break;
                }

                &Specifier::Format(c, ref padding) => {
                    self.format_format(c, &padding, width, &mut result);
                }

                &Specifier::Text(s) => {
                    if width == 0 {
                        result.push_str(s);
                    } else {
                        let remaining = width as usize - result.length();
                        let count = utils::graphemes_count(&s);
                        if remaining >= count {
                            result.push_str(&s);
                        } else {
                            result.push_str(&utils::take_graphemes(s, remaining));
                        }
                    }
                }

                &Specifier::Conditional(cond, ref then, ref els) => {
                    self.format_conditional(cond, &then, &els, width, &mut result)
                }
            }
        }

        result.into_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_do_format_replaces_variables_with_values_one_variable() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "AAA".to_string());

        assert_eq!(fmt.do_format("%?a?%a&no?", 0), "AAA");
        assert_eq!(fmt.do_format("%?b?%b&no?", 0), "no");
        assert_eq!(fmt.do_format("%?a?[%-4a]&no?", 0), "[AAA ]");
    }

    #[test]
    fn t_do_format_replaces_variables_with_values_two_variables() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "AAA".to_string());
        fmt.register_fmt('b', "BBB".to_string());

        assert_eq!(
            fmt.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert", 0),
            "asdf | AAA | BBBAAA | qwert"
        );
        assert_eq!(fmt.do_format("%?c?asdf?", 0), "");
    }

    #[test]
    fn t_do_format_replaces_variables_with_values_three_variables() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "AAA".to_string());
        fmt.register_fmt('b', "BBB".to_string());
        fmt.register_fmt('c', "CCC".to_string());

        assert_eq!(fmt.do_format("", 0), "");
        // illegal single %
        assert_eq!(fmt.do_format("%", 0), "");
        assert_eq!(fmt.do_format("%%", 0), "%");
        assert_eq!(fmt.do_format("%a%b%c", 0), "AAABBBCCC");
        assert_eq!(fmt.do_format("%%%a%%%b%%%c%%", 0), "%AAA%BBB%CCC%");

        assert_eq!(fmt.do_format("%4a", 0), " AAA");
        assert_eq!(fmt.do_format("%-4a", 0), "AAA ");

        assert_eq!(fmt.do_format("%2a", 0), "AA");
        assert_eq!(fmt.do_format("%-2a", 0), "AA");

        assert_eq!(
            fmt.do_format("<%a> <%5b> | %-5c%%", 0),
            "<AAA> <  BBB> | CCC  %"
        );
        assert_eq!(
            fmt.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert", 0),
            "asdf | AAA | AAABBB | qwert"
        );

        assert_eq!(fmt.do_format("%>X", 3), "XXX");
        assert_eq!(fmt.do_format("%a%> %b", 10), "AAA    BBB");
        assert_eq!(fmt.do_format("%a%> %b", 0), "AAA BBB");

        assert_eq!(fmt.do_format("%?c?asdf?", 0), "asdf");
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_conditional_with_one_variable() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());

        assert_eq!(fmt.do_format("%?a?%a&no?", 0), "АБВ");
        assert_eq!(fmt.do_format("%?b?%b&no?", 0), "no");
        assert_eq!(fmt.do_format("%?a?[%-4a]&no?", 0), "[АБВ ]");
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_misc_tests_with_two_variables() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());
        fmt.register_fmt('b', "буква".to_string());

        assert_eq!(
            fmt.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert", 0),
            "asdf | АБВ | букваАБВ | qwert"
        );
        assert_eq!(fmt.do_format("%?c?asdf?", 0), "");
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_simple_cases_with_three_variables() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());
        fmt.register_fmt('b', "буква".to_string());
        fmt.register_fmt('c', "ещё одна переменная".to_string());

        assert_eq!(fmt.do_format("", 0), "");
        // illegal single %
        assert_eq!(fmt.do_format("%", 0), "");
        assert_eq!(fmt.do_format("%%", 0), "%");
        assert_eq!(
            fmt.do_format("%a%b%c", 0),
            "АБВбукваещё одна переменная"
        );
        assert_eq!(
            fmt.do_format("%%%a%%%b%%%c%%", 0),
            "%АБВ%буква%ещё одна переменная%"
        );
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_alignment() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());
        fmt.register_fmt('b', "буква".to_string());
        fmt.register_fmt('c', "ещё одна переменная".to_string());

        assert_eq!(fmt.do_format("%4a", 0), " АБВ");
        assert_eq!(fmt.do_format("%-4a", 0), "АБВ ");

        assert_eq!(fmt.do_format("%2a", 0), "АБ");
        assert_eq!(fmt.do_format("%-2a", 0), "АБ");
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_complex_format_string() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());
        fmt.register_fmt('b', "буква".to_string());
        fmt.register_fmt('c', "ещё одна переменная".to_string());

        assert_eq!(
            fmt.do_format("<%a> <%5b> | %-5c%%", 0),
            "<АБВ> <буква> | ещё о%"
        );
        assert_eq!(
            fmt.do_format("asdf | %a | %?c?%a%b&%b%a? | qwert", 0),
            "asdf | АБВ | АБВбуква | qwert"
        );
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_format_string_fillers() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());
        fmt.register_fmt('b', "буква".to_string());
        fmt.register_fmt('c', "ещё одна переменная".to_string());

        assert_eq!(fmt.do_format("%>X", 3), "XXX");
        assert_eq!(fmt.do_format("%a%> %b", 10), "АБВ  буква");
        assert_eq!(fmt.do_format("%a%> %b", 0), "АБВ буква");
    }

    #[test]
    fn t_do_format_supports_multibyte_characters_conditional_format_with_three_variables() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "АБВ".to_string());
        fmt.register_fmt('b', "буква".to_string());
        fmt.register_fmt('c', "ещё одна переменная".to_string());

        assert_eq!(fmt.do_format("%?c?asdf?", 0), "asdf");
    }

    #[test]
    fn t_do_format_ignores_start_of_conditional_at_the_end_of_format_string() {
        let fmt = FmtStrFormatter::new();
        assert_eq!(fmt.do_format("%?", 0), "");
    }

    #[test]
    fn t_do_format_pads_values_on_the_left_undefined_char() {
        let fmt = FmtStrFormatter::new();

        assert_eq!(fmt.do_format("%4a", 0), "    ");
        assert_eq!(fmt.do_format("%8a", 0), "        ");
    }

    #[test]
    fn t_do_format_pads_values_on_the_left() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "hello".to_string());
        fmt.register_fmt('r', "привет".to_string());

        assert_eq!(fmt.do_format("%8a", 0), "   hello");
        assert_eq!(fmt.do_format("%5a", 0), "hello");
        assert_eq!(fmt.do_format("%8r", 0), "  привет");
        assert_eq!(fmt.do_format("%6r", 0), "привет");

        // If the value is bigger than the padding, only first N characters are used
        assert_eq!(fmt.do_format("%4a", 0), "hell");
        assert_eq!(fmt.do_format("%2a", 0), "he");
        assert_eq!(fmt.do_format("%4r", 0), "прив");
        assert_eq!(fmt.do_format("%3r", 0), "при");
    }

    #[test]
    fn t_do_format_pads_values_on_the_right_undefined_char() {
        let fmt = FmtStrFormatter::new();

        assert_eq!(fmt.do_format("%-4a", 0), "    ");
        assert_eq!(fmt.do_format("%-8a", 0), "        ");
    }

    #[test]
    fn t_do_format_pads_values_on_the_right() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "hello".to_string());
        fmt.register_fmt('r', "привет".to_string());

        assert_eq!(fmt.do_format("%-8a", 0), "hello   ");
        assert_eq!(fmt.do_format("%-5a", 0), "hello");
        assert_eq!(fmt.do_format("%-8r", 0), "привет  ");
        assert_eq!(fmt.do_format("%-6r", 0), "привет");

        // If the value is bigger than the padding, only first N characters are used
        assert_eq!(fmt.do_format("%-4a", 0), "hell");
        assert_eq!(fmt.do_format("%-2a", 0), "he");
        assert_eq!(fmt.do_format("%-4r", 0), "прив");
        assert_eq!(fmt.do_format("%-3r", 0), "при");
    }

    #[test]
    fn t_spacer_pads_consecutive_text_to_the_right_using_specified_char_total_length() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('x', "example string".to_string());
        fmt.register_fmt('y', "пример строки".to_string());

        // The total length of the string is specified in the argument to do_format

        // Default (zero) is \"as long as needed to fit all the values\"
        assert_eq!(
            fmt.do_format("%x%> %y", 0),
            "example string пример строки"
        );

        assert_eq!(
            fmt.do_format("%x%> %y", 30),
            "example string   пример строки"
        );
        assert_eq!(
            fmt.do_format("%x%> %y", 45),
            "example string                  пример строки"
        );
    }

    #[test]
    fn t_spacer_pads_consecutive_text_to_the_right_using_specified_char_check_char() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('x', "example string".to_string());
        fmt.register_fmt('y', "пример строки".to_string());

        assert_eq!(
            fmt.do_format("%x%>m%y", 0),
            "example stringmпример строки"
        );
        assert_eq!(
            fmt.do_format("%x%>k%y", 30),
            "example stringkkkпример строки"
        );
        assert_eq!(
            fmt.do_format("%x%>i%y", 45),
            "example stringiiiiiiiiiiiiiiiiiiпример строки"
        );
    }

    #[test]
    fn t_spacer_pads_consecutive_text_to_the_right_using_specified_char_only_first_one_works() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('x', "example string".to_string());
        fmt.register_fmt('y', "пример строки".to_string());
        fmt.register_fmt('s', "_short_".to_string());

        let format = "%s%>a%s%>b%s";
        assert_eq!(fmt.do_format(format, 0), "_short_a_short_b_short_");
        assert_eq!(fmt.do_format(format, 30), "_short_aaaaaaaa_short_b_short_");
    }

    #[test]
    fn t_conditional_is_replaced_by_appropriate_branch_standard_case() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('t', "this is a non-empty string".to_string());
        fmt.register_fmt('m', String::new());

        assert_eq!(fmt.do_format("%?t?non-empty&empty?", 0), "non-empty");
        assert_eq!(fmt.do_format("%?m?non-empty&empty?", 0), "empty");
        assert_eq!(
            fmt.do_format("%?t?непустое&пустое?", 0),
            "непустое"
        );
        assert_eq!(
            fmt.do_format("%?m?непустое&пустое?", 0),
            "пустое"
        );
    }

    #[test]
    fn t_conditional_is_replaced_by_appropriate_branch_else_branch_is_optonal() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('t', "this is a non-empty string".to_string());
        fmt.register_fmt('m', String::new());

        assert_eq!(
            fmt.do_format("%?t?непустое?", 0),
            "непустое"
        );
        assert_eq!(fmt.do_format("%?m?непустое?", 0), "");
    }

    #[test]
    fn t_do_format_replaces_double_percent_sign_with_a_percent_sign() {
        let fmt = FmtStrFormatter::new();
        assert_eq!(fmt.do_format("%%", 0), "%");
    }

    #[test]
    fn t_ampersand_is_treated_literally_outside_of_conditionals() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "A".to_string());
        fmt.register_fmt('b', "B".to_string());

        assert_eq!(
            fmt.do_format("%a & %b were sitting on a pipe", 0),
            "A & B were sitting on a pipe"
        );
    }

    #[test]
    fn t_question_mark_is_treated_literally_outside_conditionals() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('x', "What's the ultimate answer".to_string());
        fmt.register_fmt('y', "42".to_string());

        assert_eq!(fmt.do_format("%x? %y", 0), "What's the ultimate answer? 42");
    }

    proptest! {
        #[test]
        fn does_not_crash_when_formatting_with_no_formats_registered(ref input in "\\PC*") {
            let fmt = FmtStrFormatter::new();
            fmt.do_format(&input, 0);
        }

        #[test]
        fn does_not_crash_on_any_spacing_character(ref c in "\\PC") {
            let fmt = FmtStrFormatter::new();
            let format = format!("%>{}", c);
            fmt.do_format(&format, 0);
        }

        #[test]
        fn does_not_crash_on_any_character_in_conditional(ref c in "\\PC") {
            let fmt = FmtStrFormatter::new();
            let format = format!("%?{}?hello?%", c);
            fmt.do_format(&format, 0);
        }

        #[test]
        fn result_is_never_longer_than_specified_width(length in 1u32..10000, ref input in "\\PC*") {
            let fmt = FmtStrFormatter::new();
            let result = fmt.do_format(&input, length);
            assert!(utils::graphemes_count(&result) <= length as usize);
        }
    }
}
