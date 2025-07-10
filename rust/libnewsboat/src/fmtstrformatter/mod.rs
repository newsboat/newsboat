//! Produces strings of values in a specified format, strftime(3)-like.

mod limited_string;
mod parser;

use crate::utils;
use limited_string::LimitedString;
use parser::{parse, Padding, Specifier};
use std::collections::BTreeMap;

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
/// ```no_run
/// use libNewsboat::fmtstrformatter::*;
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
#[derive(Default)]
pub struct FmtStrFormatter {
    /// Stores keys and their values.
    fmts: BTreeMap<char, String>,
}

struct StringParts {
    head: LimitedString,
    spaced_tail: Option<(char, LimitedString)>,
    width: Option<usize>,
}

impl StringParts {
    fn new(width: Option<usize>) -> Self {
        Self {
            head: LimitedString::new(width),
            spaced_tail: None,
            width,
        }
    }

    fn add_spacing(&mut self, spacing: char) {
        if let Some((_, ref mut tail)) = self.spaced_tail {
            tail.push(spacing);
        } else {
            self.head.push(spacing);
            let remaining = self.width.map(|w| w.saturating_sub(self.head.length()));
            self.spaced_tail = Some((spacing, LimitedString::new(remaining)));
        }
    }

    fn push_str(&mut self, content: &str) {
        if let Some((_, ref mut tail)) = self.spaced_tail {
            tail.push_str(content);
        } else {
            self.head.push_str(content);
        }
    }

    fn join(&mut self, parts: Self) {
        if let Some((_, ref mut tail)) = self.spaced_tail {
            tail.push_str(&parts.head.into_string());
            if let Some((_, rest)) = parts.spaced_tail {
                tail.push_str(&rest.into_string());
            }
        } else {
            self.head.push_str(&parts.head.into_string());
            self.spaced_tail = parts.spaced_tail;
        }
    }

    fn into_string(self) -> String {
        let mut result = self.head;
        if let Some((spacing, rest)) = self.spaced_tail {
            if let Some(width) = self.width {
                let padding_width = width
                    .saturating_sub(result.length())
                    .saturating_sub(rest.length());
                if padding_width > 0 {
                    let padding_value = &format!("{spacing}");
                    let padding = padding_value.repeat(padding_width);
                    result.push_str(&padding);
                }
            }
            result.push_str(&rest.into_string());
        }
        result.into_string()
    }
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
        self.formatting_helper(&ast, width).into_string()
    }

    fn format_format(&self, c: char, padding: &Padding, width: u32, result: &mut StringParts) {
        let empty_string = String::new();
        let value = self.fmts.get(&c).unwrap_or(&empty_string);
        match *padding {
            Padding::None => result.push_str(value),

            Padding::Left(total_width) => {
                let text = &utils::substr_with_width(value, total_width);
                let padding_width = total_width - utils::strwidth(text);
                let padding = String::from(" ").repeat(padding_width);
                result.push_str(&padding);
                result.push_str(text);
            }

            Padding::Right(total_width) => {
                let text = &utils::substr_with_width(value, total_width);
                let padding_width = total_width - utils::strwidth(text);
                let padding = String::from(" ").repeat(padding_width);
                result.push_str(text);
                result.push_str(&padding);
            }

            Padding::Center(total_width) => {
                let w = if total_width == 0 {
                    width as usize
                } else {
                    total_width
                };
                let text = &utils::substr_with_width(value, w);
                let padding_width = w - utils::strwidth(text);
                if padding_width > 0 {
                    let left: usize = padding_width / 2;
                    let right: usize = padding_width - left;
                    let padding_l = String::from(" ").repeat(left);
                    let padding_r = String::from(" ").repeat(right);
                    result.push_str(&padding_l);
                    result.push_str(text);
                    result.push_str(&padding_r);
                } else {
                    result.push_str(text);
                }
            }
        }
    }

    fn format_conditional(
        &self,
        cond: char,
        then: &[Specifier],
        els: &Option<Vec<Specifier>>,
        width: u32,
        result: &mut StringParts,
    ) {
        match self.fmts.get(&cond) {
            Some(value) if !value.trim().is_empty() => {
                result.join(self.formatting_helper(then, width));
            }
            _ => {
                if let Some(ref els) = *els {
                    result.join(self.formatting_helper(els, width));
                }
            }
        }
    }

    fn formatting_helper(&self, format_ast: &[Specifier], width: u32) -> StringParts {
        let mut result = StringParts::new((width != 0).then_some(width as usize));

        for specifier in format_ast.iter() {
            match *specifier {
                Specifier::Spacing(c) => result.add_spacing(c),
                Specifier::Format(c, ref padding) => {
                    self.format_format(c, padding, width, &mut result)
                }
                Specifier::Text(s) => result.push_str(s),
                Specifier::Conditional(cond, ref then, ref els) => {
                    self.format_conditional(cond, then, els, width, &mut result)
                }
            }
        }

        result
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn t_do_format_center() {
        let mut fmt = FmtStrFormatter::new();
        fmt.register_fmt('T', "whatever".to_string());
        assert_eq!(fmt.do_format("%=20T", 0), "      whatever      ");
        assert_eq!(fmt.do_format("%=19T", 0), "     whatever      ");
        assert_eq!(fmt.do_format("%=3T", 0), "wha");
        assert_eq!(fmt.do_format("%=0T", 20), "      whatever      ");
    }

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
        assert_eq!(fmt.do_format("%a%b%c", 0), "АБВбукваещё одна переменная");
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
    fn t_do_format_keeps_wide_characters_within_specified_width() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "ＡＢＣ".to_string());
        fmt.register_fmt('b', "def".to_string());

        assert_eq!(fmt.do_format("%a %b", 0), "ＡＢＣ def");
        assert_eq!(fmt.do_format("%a %b", 10), "ＡＢＣ def");
        assert_eq!(fmt.do_format("%a %b", 9), "ＡＢＣ de");
        assert_eq!(fmt.do_format("%a %b", 7), "ＡＢＣ ");
        assert_eq!(fmt.do_format("%a %b", 6), "ＡＢＣ");
        assert_eq!(fmt.do_format("%a %b", 4), "ＡＢ");
    }

    #[test]
    fn t_do_format_does_not_include_wide_character_if_only_1_column_left() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "ＡＢＣ".to_string());

        assert_eq!(fmt.do_format("%a", 6), "ＡＢＣ");
        assert_eq!(fmt.do_format("%a", 5), "ＡＢ");
        assert_eq!(fmt.do_format("%a", 4), "ＡＢ");

        assert_eq!(fmt.do_format("%-6a", 0), "ＡＢＣ");
        assert_eq!(fmt.do_format("%-5a", 0), "ＡＢ ");
        assert_eq!(fmt.do_format("%-4a", 0), "ＡＢ");

        assert_eq!(fmt.do_format("%6a", 0), "ＡＢＣ");
        assert_eq!(fmt.do_format("%5a", 0), " ＡＢ");
        assert_eq!(fmt.do_format("%4a", 0), "ＡＢ");
    }

    #[test]
    fn t_do_format_does_not_escape_less_than_signs_in_regular_text() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "AAA".to_string());
        fmt.register_fmt('b', "BBB".to_string());

        assert_eq!(fmt.do_format("%a <%b>", 0), "AAA <BBB>");
    }

    #[test]
    fn t_do_format_does_not_escape_less_than_signs_in_filling_format() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', "AAA".to_string());
        fmt.register_fmt('b', "BBB".to_string());

        assert_eq!(fmt.do_format("%a%>.%b", 10), "AAA....BBB");
        assert_eq!(fmt.do_format("%a%><%b", 10), "AAA<<<<BBB");
        assert_eq!(fmt.do_format("%a%>>%b", 10), "AAA>>>>BBB");
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
        assert_eq!(fmt.do_format("%x%> %y", 0), "example string пример строки");

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

        assert_eq!(fmt.do_format("%x%>m%y", 0), "example stringmпример строки");
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
    fn t_spacer_pads_to_correct_length_inside_conditional_branch() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('x', "non-empty".to_string());
        fmt.register_fmt('y', "".to_string());

        assert_eq!(fmt.do_format("A%?x?%>a&%>b?B", 7), "AaaaaaB");
        assert_eq!(fmt.do_format("A%?y?%>a&%>b?B", 7), "AbbbbbB");
        assert_eq!(fmt.do_format("A%>_%?x?%>a&%>b?B", 7), "A____aB");
        assert_eq!(fmt.do_format("A%>_%?y?%>a&%>b?B", 7), "A____bB");
        assert_eq!(fmt.do_format("A%?x?%>a&%>b?%>_B", 7), "Aaaaa_B");
        assert_eq!(fmt.do_format("A%?y?%>a&%>b?%>_B", 7), "Abbbb_B");
    }

    #[test]
    fn t_conditional_is_replaced_by_appropriate_branch_standard_case() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('t', "this is a non-empty string".to_string());
        fmt.register_fmt('m', String::new());

        assert_eq!(fmt.do_format("%?t?non-empty&empty?", 0), "non-empty");
        assert_eq!(fmt.do_format("%?m?non-empty&empty?", 0), "empty");
        assert_eq!(fmt.do_format("%?t?непустое&пустое?", 0), "непустое");
        assert_eq!(fmt.do_format("%?m?непустое&пустое?", 0), "пустое");
    }

    #[test]
    fn t_conditional_is_replaced_by_appropriate_branch_else_branch_is_optonal() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('t', "this is a non-empty string".to_string());
        fmt.register_fmt('m', String::new());

        assert_eq!(fmt.do_format("%?t?непустое?", 0), "непустое");
        assert_eq!(fmt.do_format("%?m?непустое?", 0), "");
    }

    #[test]
    fn t_conditional_whitespace_is_handled_as_empty() {
        let mut fmt = FmtStrFormatter::new();

        fmt.register_fmt('a', " \t ".to_string());
        fmt.register_fmt('b', "  some whitespace  ".to_string());

        assert_eq!(fmt.do_format("%?a?non-empty&empty?", 0), "empty");
        assert_eq!(fmt.do_format("%?b?non-empty&empty?", 0), "non-empty");
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

    proptest::proptest! {
        #[test]
        fn does_not_crash_when_formatting_with_no_formats_registered(ref input in "\\PC*") {
            let fmt = FmtStrFormatter::new();
            fmt.do_format(input, 0);
        }

        #[test]
        fn does_not_crash_on_any_spacing_character(ref c in "\\PC") {
            let fmt = FmtStrFormatter::new();
            let format = format!("%>{c}");
            fmt.do_format(&format, 0);
        }

        #[test]
        fn does_not_crash_on_any_character_in_conditional(ref c in "\\PC") {
            let fmt = FmtStrFormatter::new();
            let format = format!("%?{c}?hello?%");
            fmt.do_format(&format, 0);
        }

        #[test]
        fn result_is_never_longer_than_specified_width(length in 1u32..10000, ref input in "\\PC*") {
            let fmt = FmtStrFormatter::new();
            let result = fmt.do_format(input, length);
            assert!(utils::strwidth(&result) <= length as usize);
        }
    }
}
