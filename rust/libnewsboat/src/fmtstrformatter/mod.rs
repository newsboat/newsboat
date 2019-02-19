//! Produces strings of values in a specified format, strftime(3)-like.
mod parser;

use self::parser::{parse, Padding, Specifier};
use std::cmp::min;
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

    fn format_spacing(&self, c: char, rest: &[Specifier], width: u32, result: &mut String) {
        if width == 0 {
            result.push(c);
        } else {
            let rest = self.formatting_helper(rest, 0);
            let padding_width = width as usize - rest.len() - result.len();

            let padding = format!("{}", c).repeat(padding_width);
            result.push_str(&padding);
        };
    }

    fn format_format(&self, c: char, padding: &Padding, _width: u32, result: &mut String) {
        if let Some(value) = self.fmts.get(&c) {
            match padding {
                Padding::None => result.push_str(value),

                Padding::Left(total_width) => {
                    use std::dbg;
                    let padding_width =
                        dbg!(total_width) - dbg!(min(*total_width, dbg!(value.len())));
                    let stripping_width = total_width - padding_width;
                    let padding = String::from(" ").repeat(padding_width);
                    result.push_str(&padding);
                    result.push_str(&value[0..stripping_width]);
                }

                Padding::Right(total_width) => {
                    use std::dbg;
                    let padding_width =
                        dbg!(total_width) - dbg!(min(*total_width, dbg!(value.len())));
                    let stripping_width = total_width - padding_width;
                    let padding = String::from(" ").repeat(padding_width);
                    result.push_str(&value[0..stripping_width]);
                    result.push_str(&padding);
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
        result: &mut String,
    ) {
        match self.fmts.get(&cond) {
            Some(value) if !value.is_empty() => {
                result.push_str(&self.formatting_helper(then, width))
            }
            _ => {
                if let Some(els) = els {
                    result.push_str(&self.formatting_helper(els, width))
                }
            }
        }
    }

    fn formatting_helper(&self, format_ast: &[Specifier], width: u32) -> String {
        let mut result = String::new();

        for (i, specifier) in format_ast.iter().enumerate() {
            match specifier {
                Specifier::Spacing(c) => {
                    let rest = &format_ast[i + 1..];
                    self.format_spacing(*c, rest, width, &mut result);
                }

                Specifier::Format(c, padding) => {
                    self.format_format(*c, padding, width, &mut result);
                }

                Specifier::Text(s) => result.push_str(s),

                Specifier::Conditional(cond, then, els) => {
                    self.format_conditional(*cond, then, els, width, &mut result)
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

    /*
    TEST_CASE("do_format supports multibyte characters", "[FmtStrFormatter]") {
            FmtStrFormatter fmt;

            SECTION("One format variable")
            {
                    fmt.register_fmt('a', "АБВ");

                    SECTION("Conditional format strings")
                    {
                            REQUIRE(fmt.do_format("%?a?%a&no?") == "АБВ");
                            REQUIRE(fmt.do_format("%?b?%b&no?") == "no");
                            REQUIRE(fmt.do_format("%?a?[%-4a]&no?") == "[АБВ ]");
                    }

                    SECTION("Two format variables")
                    {
                            fmt.register_fmt('b', "буква");

                            REQUIRE(fmt.do_format(
                                            "asdf | %a | %?c?%a%b&%b%a? | qwert") ==
                                    "asdf | АБВ | букваАБВ | qwert");
                            REQUIRE(fmt.do_format("%?c?asdf?") == "");

                            SECTION("Three format variables")
                            {
                                    fmt.register_fmt('c', "ещё одна переменная");

                                    SECTION("Simple cases")
                                    {
                                            REQUIRE(fmt.do_format("") == "");
                                            // illegal single %
                                            REQUIRE(fmt.do_format("%") == "");
                                            REQUIRE(fmt.do_format("%%") == "%");
                                            REQUIRE(fmt.do_format("%a%b%c") ==
                                                    "АБВбукваещё одна переменная");
                                            REQUIRE(fmt.do_format(
                                                            "%%%a%%%b%%%c%%") ==
                                                    "%АБВ%буква%ещё одна переменная%");
                                    }

                                    SECTION("Alignment")
                                    {
                                            REQUIRE(fmt.do_format("%4a") == " АБВ");
                                            REQUIRE(fmt.do_format("%-4a") ==
                                                    "АБВ ");

                                            SECTION("Alignment limits")
                                            {
                                                    REQUIRE(fmt.do_format("%2a") ==
                                                            "АБ");
                                                    REQUIRE(fmt.do_format("%-2a") ==
                                                            "АБ");
                                            }
                                    }

                                    SECTION("Complex format string")
                                    {
                                            REQUIRE(fmt.do_format("<%a> <%5b> | "
                                                                  "%-5c%%") ==
                                                    "<АБВ> <буква> | ещё о%");
                                            REQUIRE(fmt.do_format("asdf | %a | "
                                                                  "%?c?%a%b&%b%a? "
                                                                  "| qwert") ==
                                                    "asdf | АБВ | АБВбуква | qwert");
                                    }

                                    SECTION("Format string fillers")
                                    {
                                            REQUIRE(fmt.do_format("%>X", 3) ==
                                                    "XXX");
                                            REQUIRE(fmt.do_format("%a%> %b", 10) ==
                                                    "АБВ  буква");
                                            REQUIRE(fmt.do_format("%a%> %b", 0) ==
                                                    "АБВ буква");
                                    }

                                    SECTION("Conditional format string")
                                    {
                                            REQUIRE(fmt.do_format("%?c?asdf?") ==
                                                    "asdf");
                                    }
                            }
                    }
            }
    }
    */

    /*
    TEST_CASE("do_format ignores \"%?\" at the end of the format string (which "
                    "looks like a conditional but really isn't",
                    "[FmtStrFormatter]")
    {
            FmtStrFormatter fmt;
            REQUIRE(fmt.do_format("%?") == "");
    }
    */

    /*
    TEST_CASE("do_format ignores the format at the end of the format string that "
                    "looks like a conditional but lacks a \"then\" branch",
                    "[FmtStrFormatter]")
    {
            FmtStrFormatter fmt;
            REQUIRE(fmt.do_format("%?a?this is a conditional. Or is it") == "");
            REQUIRE(fmt.do_format("%?x?Это условный формат, не правда ли") == "");
    }
    */

    /*
    TEST_CASE("do_format replaces %Nx with the value of \"x\", padded "
                    "with spaces on the left to fit N columns",
                    "[FmtStrFormatter]")
    {
            FmtStrFormatter fmt;

            SECTION("Undefined format char") {
                    REQUIRE(fmt.do_format("%4a") == "    ");
                    REQUIRE(fmt.do_format("%8a") == "        ");
            }

            SECTION("Defined format char") {
                    fmt.register_fmt('a', "hello");
                    fmt.register_fmt('r', "привет");

                    REQUIRE(fmt.do_format("%8a") == "   hello");
                    REQUIRE(fmt.do_format("%5a") == "hello");
                    REQUIRE(fmt.do_format("%8r") == "  привет");
                    REQUIRE(fmt.do_format("%6r") == "привет");

                    {
                            INFO("If the value is bigger than the padding, only first N "
                                            "characters are used");
                            REQUIRE(fmt.do_format("%4a") == "hell");
                            REQUIRE(fmt.do_format("%2a") == "he");
                            REQUIRE(fmt.do_format("%4r") == "прив");
                            REQUIRE(fmt.do_format("%3r") == "при");
                    }
            }
    }
    */

    /*
    TEST_CASE("do_format replaces %-Nx with the value of \"x\", padded "
                    "with spaces on the right to fit N columns",
                    "[FmtStrFormatter]")
    {
            FmtStrFormatter fmt;

            SECTION("Undefined format char") {
                    REQUIRE(fmt.do_format("%-4a") == "    ");
                    REQUIRE(fmt.do_format("%-8a") == "        ");
            }

            SECTION("Defined format char") {
                    fmt.register_fmt('a', "hello");
                    fmt.register_fmt('r', "привет");

                    REQUIRE(fmt.do_format("%-8a") == "hello   ");
                    REQUIRE(fmt.do_format("%-5a") == "hello");
                    REQUIRE(fmt.do_format("%-8r") == "привет  ");
                    REQUIRE(fmt.do_format("%-6r") == "привет");

                    {
                            INFO("If the value is bigger than the padding, only first N "
                                            "characters are used");
                            REQUIRE(fmt.do_format("%-4a") == "hell");
                            REQUIRE(fmt.do_format("%-2a") == "he");
                            REQUIRE(fmt.do_format("%-4r") == "прив");
                            REQUIRE(fmt.do_format("%-3r") == "при");
                    }
            }
    }
    */

    /*
    TEST_CASE("%>[char] pads consecutive text to the right, using [char] for "
                    "the padding",
                    "[FmtStrFormatter]")
    {
            FmtStrFormatter fmt;

            fmt.register_fmt('x', "example string");
            fmt.register_fmt('y', "пример строки");

            SECTION("The total length of the string is specified in the argument to do_format") {
                    {
                            INFO("Default (zero) is \"as long as needed to fit all the values\"");
                            REQUIRE(fmt.do_format("%x%> %y") == "example string пример строки");
                            REQUIRE(fmt.do_format("%x%> %y", 0) == "example string пример строки");
                    }

                    REQUIRE(fmt.do_format("%x%> %y", 30) == "example string   пример строки");
                    REQUIRE(fmt.do_format("%x%> %y", 45)
                                    == "example string                  пример строки");
            }

            SECTION("[char] is used for padding") {
                    REQUIRE(fmt.do_format("%x%>m%y") == "example stringmпример строки");
                    REQUIRE(fmt.do_format("%x%>f%y", 0) == "example stringfпример строки");
                    REQUIRE(fmt.do_format("%x%>k%y", 30) == "example stringkkkпример строки");
                    REQUIRE(fmt.do_format("%x%>i%y", 45)
                                    == "example stringiiiiiiiiiiiiiiiiiiпример строки");
            }

            SECTION("If multiple %>[char] specifiers are used, the first one has a "
                            "maximal width and the others have the width of one column")
            {
                    fmt.register_fmt('s', "_short_");

                    const auto format = std::string("%s%>a%s%>b%s");
                    REQUIRE(fmt.do_format(format) == "_short_a_short_b_short_");
                    REQUIRE(fmt.do_format(format, 30) == "_short_aaaaaaaa_short_b_short_");
            }
    }
    */

    /*
    TEST_CASE("%?[char]?[then-format]&[else-format]? is replaced by "
                    "\"then-format\" if \"[char]\" has non-empty value, otherwise it's "
                    "replaced by \"else-format\"",
                    "[FmtStrFormatter]")
    {
            FmtStrFormatter fmt;

            fmt.register_fmt('t', "this is a non-empty string");
            fmt.register_fmt('m', "");

            SECTION("Standard case") {
                    REQUIRE(fmt.do_format("%?t?non-empty&empty?") == "non-empty");
                    REQUIRE(fmt.do_format("%?m?non-empty&empty?") == "empty");
                    REQUIRE(fmt.do_format("%?t?непустое&пустое?") == "непустое");
                    REQUIRE(fmt.do_format("%?m?непустое&пустое?") == "пустое");
            }

            SECTION("Else-format is optional") {
                    REQUIRE(fmt.do_format("%?t?непустое?") == "непустое");
                    REQUIRE(fmt.do_format("%?m?непустое?") == "");
            }
    }
    */

    #[test]
    fn t_do_format_replaces_double_percent_sign_with_a_percent_sign() {
        let fmt = FmtStrFormatter::new();
        assert_eq!(fmt.do_format("%%", 0), "%");
    }

    /*
    TEST_CASE("Nested conditionals confuse do_format", "[FmtStrFormatter]") {
            FmtStrFormatter fmt;

            fmt.register_fmt('x', "unknown");
            fmt.register_fmt('y', "why");

            REQUIRE(fmt.do_format("%?x?   %?y?y-nonempty&y-empty?   &x-empty?")
                            == "   y?y-nonempty&y-empty?   &x-empty?");
            REQUIRE(fmt.do_format("%?x?   %?y?y-nonempty?   &x-empty?")
                            == "   y?y-nonempty?   &x-empty?");
    }
    */

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
}
